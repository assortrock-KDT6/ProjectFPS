#include "UI/Session/SessionMenuWidget.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "GameInstance/SessionMapCatalog.h"
#include "GameMode/FPSLobbyGameState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "InputCoreTypes.h"

void USessionMenuWidget::NativeConstruct()
{
    _SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>() : nullptr;

    BindSessionEvents();

    if (UWorld* World = GetWorld())
    {
        _GameStateSetHandle = World->GameStateSetEvent.AddUObject(this, &ThisClass::ObserveGameState);
        ObserveGameState(World->GetGameState());
    }

    _Notice = IsValid(_SessionSubsystem) ? _SessionSubsystem->GetLastSessionError() : TEXT("온라인 세션 서브시스템을 사용할 수 없습니다.");

    _NoticeIsError = false == _Notice.IsEmpty();

    // Make the initial snapshot available to Blueprint Construct.
    UpdateDisplay();

    Super::NativeConstruct();

    _ViewReady = true;

    ClearSearchResults();

    UpdateDisplay();
}

void USessionMenuWidget::NativeDestruct()
{
    _ViewReady = false;
    
    UnbindSessionEvents();
    
    if (GetWorld()) 
    { 
        GetWorld()->GameStateSetEvent.Remove(_GameStateSetHandle); 
    }

    _GameStateSetHandle.Reset();
    
    ObserveGameState(nullptr);
    
    _SearchResults.Reset();
    
    Super::NativeDestruct();
}

void USessionMenuWidget::ObserveGameState(AGameStateBase* GameState)
{
    if (true == IsValid(_ObservedLobby)) 
    { 
        _ObservedLobby->_OnLobbyChanged.RemoveDynamic(this, &ThisClass::UpdateDisplay); 
    }

    _ObservedLobby = Cast<AFPSLobbyGameState>(GameState);

    if (IsValid(_ObservedLobby)) 
    { 
        _ObservedLobby->_OnLobbyChanged.AddUniqueDynamic(this, &ThisClass::UpdateDisplay);
    }

    UpdateDisplay();
}

FReply USessionMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() == EKeys::Escape)
    {
        ProcessCloseAction();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

bool USessionMenuWidget::IsBusy() const
{
    return !IsValid(_SessionSubsystem) || _SessionSubsystem->IsExitInProgress()
        || _SessionSubsystem->IsBusy() || _SessionSubsystem->IsAutoMatchInProgress();
}

bool USessionMenuWidget::CanBrowse() const
{
    return !IsBusy() && _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::None;
}

void USessionMenuWidget::CloseMenu()
{
    if (!IsBusy()) { _OnCloseRequested.Broadcast(); }
}

void USessionMenuWidget::RefreshSessions()
{
    if (false == CanBrowse()) 
    { 
        return;
    }

    ClearNotice();

    ClearSearchResults();

    _SessionSubsystem->FindSessions(FMath::Max(1, _MaxSearchResults));

    UpdateDisplay();
}

bool USessionMenuWidget::BuildCreateOptions(FFPSSessionCreateOptions& Options, const FString& RoomName, int32 MaxPlayers, bool AllowJoinInProgress)
{
    Options._DisplayName = RoomName.TrimStartAndEnd();

    if (Options._DisplayName.IsEmpty() || Options._DisplayName.Len() > 48)
    {
        SetNotice(TEXT("방 이름을 1~48자로 입력해 주세요."), true);
        return false;
    }

    const USessionMapCatalog* Catalog = _SessionSubsystem->GetMapCatalog();

    if (!Catalog || Catalog->LobbyLevel.IsNull())
    {
        SetNotice(TEXT("세션 맵 목록에 로비 레벨을 지정해 주세요."), true);
        return false;
    }

    Options._MaxPlayers = FMath::Clamp(MaxPlayers, 2, 16);
    Options._AllowJoinProgress = AllowJoinInProgress;
    Options._MapId = Catalog->LobbyLevel.ToSoftObjectPath().GetLongPackageName();
    Options._GameModeId = FPSMatchModeUtils::ToId(_RequestedMode);
    return true;
}

void USessionMenuWidget::CreateSession(const FString& RoomName, int32 MaxPlayers, bool AllowJoinInProgress)
{
    if (false == CanBrowse()) 
    { 
        return; 
    }

    ClearNotice();

    FFPSSessionCreateOptions Options;
    
    if (false == BuildCreateOptions(Options, RoomName, MaxPlayers, AllowJoinInProgress)) 
    { 
        return; 
    }

    ClearSearchResults();

    _SessionSubsystem->CreateSession(Options);

    UpdateDisplay();
}

void USessionMenuWidget::QuickJoin(const FString& RoomName, int32 MaxPlayers, bool AllowJoinInProgress)
{
    if (false == CanBrowse()) 
    { 
        return; 
    }

    ClearNotice();

    FFPSSessionCreateOptions Options;

    if (false == BuildCreateOptions(Options, RoomName, MaxPlayers, AllowJoinInProgress)) 
    { 
        return; 
    }

    ClearSearchResults();

    _SessionSubsystem->AutoJoinOrHost(Options, FMath::Max(1, _MaxSearchResults));

    UpdateDisplay();
}

void USessionMenuWidget::StartMatch()
{
    if (IsBusy() || _SessionSubsystem->GetConnectionState() != EFPSOnlineConnectionState::Hosting) 
    { 
        return; 
    }

    ClearNotice();

    _SessionSubsystem->StartMatch(_SessionSubsystem->GetSelectedGameMap().GetMapPath());

    UpdateDisplay();
}

void USessionMenuWidget::ConfirmLeave()
{
    if (IsBusy() || EFPSOnlineConnectionState::None == _SessionSubsystem->GetConnectionState())
    { 
        return; 
    }

    ClearNotice();
    if (!_SessionSubsystem->RequestExit(false))
    {
        SetNotice(_SessionSubsystem->GetLastSessionError(), true);
    }
    UpdateDisplay();
}

void USessionMenuWidget::SelectMode(EFPSMatchMode Mode)
{
    if (IsBusy() || (Mode != EFPSMatchMode::PVP && Mode != EFPSMatchMode::PVE)) 
    { 
        return; 
    }
    if (CanBrowse()) 
    { 
        _RequestedMode = Mode; ClearNotice(); 
    }
    else
    {
        FString Error;

        if (!_SessionSubsystem->SelectGameMode(Mode, Error)) 
        { 
            SetNotice(Error, true); 
        }
        else 
        { 
            ClearNotice();
        }
    }
    UpdateDisplay();
}

void USessionMenuWidget::SelectMap(int32 MapIndex)
{
    if (IsBusy() || !State.MapOptions.IsValidIndex(MapIndex)) 
    { 
        return;
    }

    FString Error;

    if (!_SessionSubsystem->SelectGameMap(State.MapOptions[MapIndex].GetMapPath(), Error)) 
    { 
        SetNotice(Error, true); 
    }
    else 
    { 
        ClearNotice(); 
    }

    UpdateDisplay();
}

void USessionMenuWidget::SetNotice(const FString& Message, bool IsError)
{
    _Notice = Message;
    _NoticeIsError = IsError;

    if (IsError && IsValid(_SessionSubsystem)) 
    { 
        _SessionSubsystem->SetLastSessionError(Message); 
    }

    if (true == _ViewReady) 
    { 
        RefreshView();
    }
}

void USessionMenuWidget::ClearSearchResults()
{
    _SearchResults.Reset();
    _HasSearchResults = false;

    if (true == _ViewReady) 
    { 
        RefreshResultList();
    }
}

void USessionMenuWidget::HandleFindCompleted(bool WasSuccessful, const TArray<FFPSOnlineSessionInfo>& Sessions, const FString& ErrorMessage)
{
    if (_SessionSubsystem->IsAutoMatchInProgress()) 
    { 
        return;
    }

    _SearchResults = WasSuccessful ? Sessions : TArray<FFPSOnlineSessionInfo>();

    _HasSearchResults = WasSuccessful;

    if (false == WasSuccessful) 
    { 
        SetNotice(ErrorMessage, true);
    }

    if (_ViewReady) 
    { 
        RefreshResultList();
    }

    UpdateDisplay();
}

void USessionMenuWidget::UpdateDisplay()
{
    State = FSessionMenuState();
    State.Available = IsValid(_SessionSubsystem);
    State.Ready = !IsBusy();
    State.CanBrowse = CanBrowse();
    State.ReturningToMenu = State.Available && _SessionSubsystem->IsExitInProgress();
    if (State.Available)
    {
        State.Connection = _SessionSubsystem->GetConnectionState();
        State.Operation = _SessionSubsystem->GetOperationState();
        State.Travel = _SessionSubsystem->GetTravelState();
        State.Service = _SessionSubsystem->GetOnlineServiceName();
        State.Connected = State.Connection != EFPSOnlineConnectionState::None;
        State.Host = State.Connection == EFPSOnlineConnectionState::Hosting;
        _SessionSubsystem->GetCurrentSessionInfo(State.Room);
        const auto* Lobby = GetWorld() ? GetWorld()->GetGameState<AFPSLobbyGameState>() : nullptr;
        State.CanSelectMap = State.Host && State.Ready && IsValid(Lobby);
        if (State.Connected && IsValid(Lobby))
        {
            State.SelectedMap = Lobby->GetSelectedMap();
            _RequestedMode = State.SelectedMap.Mode;
        }
        State.CanStart = State.CanSelectMap && !State.SelectedMap.Level.IsNull();
        State.Mode = _RequestedMode;
        for (const auto& Map : _SessionSubsystem->GetPlayableMaps())
        {
            if (Map.Mode == State.Mode) { State.MapOptions.Add(Map); }
        }
        State.SelectedMapIndex = State.MapOptions.IndexOfByPredicate([this](const FFPSPlayableMap& Map)
        { return Map.GetMapPath() == State.SelectedMap.GetMapPath(); });
        if (IsValid(Lobby))
        {
            for (const APlayerState* Player : Lobby->PlayerArray)
            {
                if (!IsValid(Player) || Player->IsInactive() || Player->IsOnlyASpectator()) { continue; }
                if (Player == GetOwningPlayerState()) { State.LocalPlayerIndex = State.PlayerNames.Num(); }
                State.PlayerNames.Add(Player->GetPlayerName());
            }
        }
    }
    if (_ViewReady) { RefreshView(); }
}

void USessionMenuWidget::BindSessionEvents()
{
	if (!IsValid(_SessionSubsystem))
	{
		return;
	}
	_SessionSubsystem->_OnFindSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleFindCompleted);
	_SessionSubsystem->_OnCreateSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnJoinSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnLobbyReady.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnMatchStarted.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnExitCleanupCompleted.AddUniqueDynamic(this, &ThisClass::HandleExitCompleted);
	_SessionSubsystem->_OnAutoMatchCompleted.AddUniqueDynamic(this, &ThisClass::HandleAutoMatchCompleted);
	_SessionSubsystem->_OnTravelFailed.AddUniqueDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnConnectionLost.AddUniqueDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnOperationStateChanged.AddUniqueDynamic(this, &ThisClass::HandleOperationChanged);
	_SessionSubsystem->_OnConnectionStateChanged.AddUniqueDynamic(this, &ThisClass::HandleConnectionChanged);
	_SessionSubsystem->_OnTravelStateChanged.AddUniqueDynamic(this, &ThisClass::HandleTravelChanged);
}

void USessionMenuWidget::UnbindSessionEvents()
{
	if (!IsValid(_SessionSubsystem))
	{
		return;
	}
	_SessionSubsystem->_OnFindSessionCompleted.RemoveDynamic(this, &ThisClass::HandleFindCompleted);
	_SessionSubsystem->_OnCreateSessionCompleted.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnJoinSessionCompleted.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnLobbyReady.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnMatchStarted.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnExitCleanupCompleted.RemoveDynamic(this, &ThisClass::HandleExitCompleted);
	_SessionSubsystem->_OnAutoMatchCompleted.RemoveDynamic(this, &ThisClass::HandleAutoMatchCompleted);
	_SessionSubsystem->_OnTravelFailed.RemoveDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnConnectionLost.RemoveDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnOperationStateChanged.RemoveDynamic(this, &ThisClass::HandleOperationChanged);
	_SessionSubsystem->_OnConnectionStateChanged.RemoveDynamic(this, &ThisClass::HandleConnectionChanged);
	_SessionSubsystem->_OnTravelStateChanged.RemoveDynamic(this, &ThisClass::HandleTravelChanged);
}

void USessionMenuWidget::JoinSession(int32 ResultIndex)
{
	if (!CanBrowse())
	{
		return;
	}

	bool JoinableResult = false;

	for (const FFPSOnlineSessionInfo& Session : _SearchResults)
	{
		if (Session._ResultIndex == ResultIndex && ResultIndex != INDEX_NONE
			&& Session._MaxPlayers > 0 && Session._CurrentPlayers < Session._MaxPlayers)
		{
			JoinableResult = true;
			break;
		}
	}
	if (false == JoinableResult)
	{
		SetNotice(TEXT("참가할 수 없는 검색 결과입니다. 목록을 다시 검색해 주세요."), true);
		return;
	}
	ClearNotice();
	_SessionSubsystem->JoinSessionByIndex(ResultIndex);
	UpdateDisplay();
}

void USessionMenuWidget::HandleExitCompleted(bool WasSuccessful, const FString& ErrorMessage)
{
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleRequestCompleted(bool WasSuccessful, const FString& ErrorMessage)
{
	if (_SessionSubsystem->IsAutoMatchInProgress())
	{
		return;
	}
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage)
{
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleSessionError(const FString& ErrorMessage)
{
	if (_SessionSubsystem->IsAutoMatchInProgress())
	{
		return;
	}
	SetNotice(ErrorMessage, true);
	UpdateDisplay();
}

void USessionMenuWidget::HandleOperationChanged(EFPSOnlineOperationState NewState)
{
	if (NewState == EFPSOnlineOperationState::Finding)
	{
		ClearSearchResults();
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleConnectionChanged(EFPSOnlineConnectionState NewState)
{
	if (NewState != EFPSOnlineConnectionState::None)
	{
		ClearSearchResults();
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleTravelChanged(EFPSOnlineTravelState NewState)
{
	UpdateDisplay();
}

void USessionMenuWidget::ClearNotice()
{
	SetNotice(FString());
	if (IsValid(_SessionSubsystem))
	{
		_SessionSubsystem->SetLastSessionError(FString());
	}
}

