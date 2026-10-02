#include "GameInstance/FPSLoadingSubsystem.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "GameMode/FPSGameState.h"
#include "Controller/PlayerControllerBase.h"
#include "UI/Loading/FPSLoadingWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPSLoading, Log, All);

bool UFPSLoadingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return true == Super::ShouldCreateSubsystem(Outer) && false == IsRunningDedicatedServer();
}

void UFPSLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Collection.InitializeDependency<UFPSOnlineSessionSubsystem>();

    Sessions = GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>();

    _LoadedData = ScreenData.LoadSynchronous();

    if (_LoadedData == nullptr)
    {
        // 에셋이 누락되어도 맵 이동이나 게임 플레이가 중단되어서는 안 된다.     
        UE_LOG(LogFPSLoading, Warning, TEXT("Loading ScreenData is not configured; loading UI disabled."));
        return;
    }

    if (auto* Background = _LoadedData->LobbyBackground.LoadSynchronous())
    {
        RetainedBackgrounds.AddUnique(Background);
    }

    if (Sessions && Sessions->GetMapCatalog())
    {
        for (const auto& Map : Sessions->GetPlayableMaps())
        {
            if (auto* Background = Map.Thumbnail.LoadSynchronous()) 
            {
                RetainedBackgrounds.AddUnique(Background);
            }
        }
    }

    if (nullptr != Sessions)
    {
        Sessions->_OnTravelFailed.AddDynamic(this, &ThisClass::HandleSessionFailure);

        Sessions->_OnConnectionLost.AddDynamic(this, &ThisClass::HandleSessionFailure);
    }

    //  PreLoadMapWithContext : 언리얼 엔진(UE5 이상)에서 맵(레벨)이 로드되기 직전에 호출되는 코어 엔진 델리게이트
    //  PostLoadMapWithWorld  : 언리얼 엔진에서 맵(레벨) 로딩이 완료된 시점을 처리하기 위해 제공하는 전역 델리게이트  
    PreLoadHandle   = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisClass::HandlePreLoadMap);
    PostLoadHandle  = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);

    //  FTSTicker : 지정된 지연 시간(Delay)이나 주기(Interval)에 따라 
    //              델리게이트(Delegate, 콜백 함수)를 안전하게 실행해 주는 
    //              스레드 세이프(Thread-safe) 틱 관리 시스템
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickLoading));
}

void UFPSLoadingSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);

    FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadHandle);

    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);

    if (nullptr != Sessions)
    {
        Sessions->_OnTravelFailed.RemoveDynamic(this, &ThisClass::HandleSessionFailure);

        Sessions->_OnConnectionLost.RemoveDynamic(this, &ThisClass::HandleSessionFailure);
    }

    HideScreen();

    _LoadedData = nullptr;

    RetainedBackgrounds.Reset();

    Super::Deinitialize();
}

void UFPSLoadingSubsystem::ResolveDestination(const FString& Destination)
{
    FString Package = Destination;

    Package.Split(TEXT("?"), &Package, nullptr);

    Package = UWorld::RemovePIEPrefix(Package);

    _State.MapName = FText::FromString(TEXT("로비"));

    _State.ModeName = FText::GetEmpty();

    _State.Background = _LoadedData->LobbyBackground.Get();

    if (Sessions)
    {
        TOptional<EFPSMatchMode> Mode;
        EFPSMatchMode ParsedMode;
        const int32 OptionsStart = Destination.Find(TEXT("?"));
        FString ModeId = OptionsStart == INDEX_NONE ? FString()
            : UGameplayStatics::ParseOption(Destination.Mid(OptionsStart), TEXT("SessionMode"));
        if (ModeId.IsEmpty() && GetWorld()
            && Package == UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName()))
        {
            ModeId = GetWorld()->URL.GetOption(TEXT("SessionMode="), TEXT(""));
        }
        if (ModeId.IsEmpty())
        {
            FFPSOnlineSessionInfo Info;
            if (Sessions->GetCurrentSessionInfo(Info)) ModeId = Info._GameModeId;
        }
        if (!ModeId.IsEmpty() && FPSMatchModeUtils::TryParse(ModeId, ParsedMode)) Mode = ParsedMode;
        if (const auto* Map = Sessions->FindPlayableMap(Package, Mode))
        {
            _State.MapName = Map->DisplayName;

            _State.ModeName = FText::FromString(Map->Mode == EFPSMatchMode::PVP ? TEXT("PVP") : TEXT("PVE"));

            _State.Background = Map->Thumbnail.LoadSynchronous();

            if (_State.Background)
            {
                RetainedBackgrounds.AddUnique(_State.Background);
            }

            return;
        }
        if (Sessions->GetLobbyMapPath() == Package)
        {
            return;
        }
    }

    // Join URL은 맵 패키지 경로가 아닌 서버 주소일 수 있으므로 UI에 표시하지 않는다.

    if (!Package.StartsWith(TEXT("/Game/")))
    {
        _State.MapName = FText::GetEmpty();
        return;
    }

    _State.MapName = FText::FromString(FPackageName::GetShortName(Package));
}

void UFPSLoadingSubsystem::NotifyTravelStarting(const FString& Destination)
{
    if (!_LoadedData || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    TravelSourceWorld = GetWorld();

    AwaitingDestinationWorld = true;

    _State.Phase = EFPSLoadingPhase::LoadingMap;

    _State.StatusText = NSLOCTEXT("FPSLoading", "Loading", "불러오는 중");

    _State.ReadyPlayers = 0;

    _State.ExpectedPlayers = 0;

    _State.CountdownSeconds = 0;
    
    ResolveDestination(Destination);

    ReleaseInputBlock();

    UpdateLoadingWidget(GetGameInstance()->GetFirstLocalPlayerController());

    UE_LOG(LogFPSLoading, Log, TEXT("Travel loading started: %s"), *Destination);
}

void UFPSLoadingSubsystem::HandlePreLoadMap(const FWorldContext& Context, const FString& MapName)
{
    if (Context.OwningGameInstance != GetGameInstance() || !_LoadedData || Context.WorldType == EWorldType::Editor
        || (Context.World() && Context.World()->GetNetMode() == NM_DedicatedServer))
    {
        return;
    }

    TravelSourceWorld = Context.World();

    AwaitingDestinationWorld = true;

    _State.Phase = EFPSLoadingPhase::LoadingMap;

    _State.StatusText = NSLOCTEXT("FPSLoading", "Loading", "불러오는 중");

    _State.ReadyPlayers = 0;
    _State.ExpectedPlayers = 0;
    _State.CountdownSeconds = 0;

    ResolveDestination(MapName);

    ReleaseInputBlock();

    UpdateLoadingWidget(GetGameInstance()->GetFirstLocalPlayerController());

    // LoadMap으로 게임 스레드가 블로킹되기 전에 현재 UMG 프레임을 한 번 갱신한다.
    // PIE에서는 에디터 전체 렌더링이나 중첩 Tick이 발생하지 않도록 별도로 처리한다.    
    if (Context.WorldType != EWorldType::PIE)
    {
        PresentBeforeMapLoad();
    }
}

void UFPSLoadingSubsystem::HandlePostLoadMap(UWorld* World)
{
    if (!World || World->GetGameInstance() != GetGameInstance() || !_LoadedData || World->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    ObservedWorld = World;

    AwaitingDestinationWorld = false;

    _State.Phase = EFPSLoadingPhase::PreparingPlayer;

    _State.StatusText = NSLOCTEXT("FPSLoading", "Preparing", "준비 중");

    ResolveDestination(World->GetOutermost()->GetName());

    UE_LOG(LogFPSLoading, Log, TEXT("Map loaded; waiting for local readiness: %s"), *World->GetName());

    // 맵 전환 후에도 유지되는 로딩 위젯이 새 World를 참조하도록 PlayerContext를 갱신한다.
    if (_LoadingWidget)
    {
        _LoadingWidget->SetPlayerContext(FLocalPlayerContext(GetGameInstance()->GetFirstGamePlayer(), World));
    }

    ShowLoadingWidget();
}

void UFPSLoadingSubsystem::ShowLoadingWidget()
{
    if (!_LoadedData || !_LoadedData->WaitingWidgetClass)
    {
        return;
    }

    if (!_LoadingWidget)
    {
        // GameInstance와 LocalPlayer는 맵 전환 후에도 유지되므로
        // 로딩 위젯의 생명주기를 이전 PlayerController에 종속시키지 않는다.
        _LoadingWidget = CreateWidget<UFPSLoadingWidget>(GetGameInstance(), _LoadedData->WaitingWidgetClass);
    }

    if (!_LoadingWidget)
    {
        return;
    }

    _LoadingWidget->ApplyLoadingState(_State, _LoadedData);

    if (false == _LoadingWidget->IsInViewport())
    {
        if (auto* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld()))
        {
            FGameViewportWidgetSlot Slot;

            // ESC/종료 메뉴는 ZOrder 2000으로 로딩 화면보다 위에 표시한다.
            Slot.ZOrder = 1000; 

            Slot.bAutoRemoveOnWorldRemoved = false;

            ViewportSubsystem->AddWidget(_LoadingWidget, Slot);
        }
    }

}

void UFPSLoadingSubsystem::PresentBeforeMapLoad()
{
    auto* Viewport = GetGameInstance()->GetGameViewportClient();

    if (!_LoadingWidget || !_LoadingWidget->IsInViewport() || !Viewport || !FSlateApplication::IsInitialized())
    {
        return;
    }

    // UMG는 엔진의 Slate 렌더러를 사용하므로 현재 Blueprint 위젯 프레임만 갱신한다.
    // 별도의 Slate 위젯, MoviePlayer, 엔진 Tick 또는 네트워크 준비 대기용 블로킹 루프는 사용하지 않는다.

    FSlateApplication::Get().Tick(ESlateTickType::TimeAndWidgets);
}

void UFPSLoadingSubsystem::SetInputBlocked(APlayerController* Controller)
{
    if (BlockedController.Get() == Controller)
    {
        return;
    }

    ReleaseInputBlock();

    if (Controller)
    {
        if (auto* FPSController = Cast<APlayerControllerBase>(Controller))
        {
            FPSController->SetLoadingInputBlocked(true);
        }
        else
        {
            Controller->SetIgnoreMoveInput(true);
            Controller->SetIgnoreLookInput(true);
        }

        BlockedController = Controller;
    }
}

void UFPSLoadingSubsystem::ReleaseInputBlock()
{
    if (auto* Controller = BlockedController.Get())
    {
        if (auto* FPSController = Cast<APlayerControllerBase>(Controller)) FPSController->SetLoadingInputBlocked(false);
        else
        {
            Controller->SetIgnoreMoveInput(false);
            Controller->SetIgnoreLookInput(false);
        }
    }
    BlockedController.Reset();
}

void UFPSLoadingSubsystem::UpdateLoadingWidget(APlayerController* Controller)
{
    ShowLoadingWidget();
    SetInputBlocked(Controller);
}

bool UFPSLoadingSubsystem::TickLoading(float DeltaSeconds)
{
    RefreshAccumulator += DeltaSeconds;

    if (RefreshAccumulator < 0.1f)
    {
        return true;
    }

    RefreshAccumulator = 0;

    UWorld* World = GetWorld();

    if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer) 
    {
        return true;
    }

    // Seamless Travel의 임시 전환 World를 목적지로 취급하면 로딩이 조기에 끝난다.
    // 최종 맵은 엔진의 PostLoadMapWithWorld 콜백에서 준비 상태로 전환한다.
    if (World->IsInSeamlessTravel())
    {
        return true;
    }

    // 늦게 참가한 클라이언트는 자신의 Pawn/PlayerState보다 진행 중인 매치 상태를 먼저 받을 수 있다.
    if (ObservedWorld.Get() != World && (!AwaitingDestinationWorld || TravelSourceWorld.Get() != World)) 
    {
        HandlePostLoadMap(World);
    }

    if (_State.Phase == EFPSLoadingPhase::Hidden || AwaitingDestinationWorld || !World->HasBegunPlay()) 
    {
        return true;
    }

    APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController(World);

    if (!Controller || !Controller->IsLocalController()) 
    {
        return true;
    }

    auto* GameState = World->GetGameState<AFPSGameState>();

    if (nullptr != GameState)
    {
        const bool LocalReady = Controller->PlayerState && (Controller->GetPawn() || Controller->PlayerState->IsOnlyASpectator());

        // Late joins can receive the running match state before their own pawn/player state.
        if (GameState->HasMatchStarted() && LocalReady) 
        { 
            HideScreen(); 

            return true; 
        }

        _State.ReadyPlayers = GameState->GetReadyPlayerCount();

        _State.ExpectedPlayers = GameState->GetExpectedPlayerCount();

        if (!LocalReady)
        {
            _State.Phase = EFPSLoadingPhase::PreparingPlayer;
            _State.StatusText = NSLOCTEXT("FPSLoading", "Preparing", "준비 중");
        }
        else if (GameState->IsStartCountdownActive())
        {
            _State.Phase = EFPSLoadingPhase::Countdown;
            _State.CountdownSeconds = FMath::Max(0, FMath::CeilToInt(GameState->GetRemainingStartDelay()));
            _State.StatusText = FText::Format(NSLOCTEXT("FPSLoading", "Countdown", "{0}초 후 시작"), FText::AsNumber(_State.CountdownSeconds));
        }
        else
        {
            _State.Phase = EFPSLoadingPhase::WaitingForPlayers;
            _State.StatusText = FText::Format(NSLOCTEXT("FPSLoading", "Waiting", "플레이어 대기 {0}/{1}"),
                FText::AsNumber(_State.ReadyPlayers), FText::AsNumber(_State.ExpectedPlayers));
        }

        UpdateLoadingWidget(Controller);
    }
    else if (World->GetGameState() && Controller->PlayerState)
    {
        // 로비에서는 전투용 Pawn이나 매치 카운트다운 없이
        // 복제된 Controller와 PlayerState만 준비되어도 로딩을 종료할 수 있다.        
        HideScreen();
    }
    else
    {
        UpdateLoadingWidget(Controller);
    }

    return true;
}

void UFPSLoadingSubsystem::HideScreen()
{
    const bool WasVisible = IsLoadingScreenVisible();

    if (_LoadingWidget) 
    { 
        _LoadingWidget->RemoveFromParent(); _LoadingWidget = nullptr; 
    }

    ReleaseInputBlock();

    AwaitingDestinationWorld = false;

    _State.Phase = EFPSLoadingPhase::Hidden;

    if (WasVisible)
    {
        UE_LOG(LogFPSLoading, Log, TEXT("Loading screen dismissed; input released"));
    }

}

void UFPSLoadingSubsystem::HandleSessionFailure(const FString& Error)
{
    if (!IsLoadingScreenVisible())
    {
        return;
    }
    UE_LOG(LogFPSLoading, Warning, TEXT("Loading cancelled after travel/network failure: %s"), *Error);

    HideScreen(); // 세션 정리와 사용자 오류 알림은 SessionSubsystem에서 담당한다.
}
