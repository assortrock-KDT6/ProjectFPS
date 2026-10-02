// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/PlayerControllerBase.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "GameInstance/FPSLoadingSubsystem.h"
#include "GameMode/FPSGameState.h"
#include "UI/GamePlay/MatchResultWidget.h"
#include "UI/MainHUD.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

APlayerControllerBase::APlayerControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<UMatchResultWidget> ResultWidget(
		TEXT("/Game/Blueprints/UI/Widget/GamePlay/WBP_MatchResult"));
	if (ResultWidget.Succeeded()) _MatchResultWidgetClass = ResultWidget.Class;
}

void APlayerControllerBase::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	if (IsLocalController() && GetGameInstance())
	{
		if (auto* Loading = GetGameInstance()->GetSubsystem<UFPSLoadingSubsystem>())
		{
			Loading->NotifyTravelStarting(PendingURL);
		}
	}
}

bool APlayerControllerBase::IsMoveInputIgnored() const
{
	return _LoadingInputBlocked || Super::IsMoveInputIgnored();
}

bool APlayerControllerBase::IsLookInputIgnored() const
{
	return _LoadingInputBlocked || Super::IsLookInputIgnored();
}

void APlayerControllerBase::SetLoadingInputBlocked(bool Blocked)
{
	_LoadingInputBlocked = Blocked;
}

void APlayerControllerBase::ClientShowMatchResults_Implementation(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime)
{
	if (!IsLocalController())
	{
		return;
	}

	if (IsValid(_MatchResultWidget))
	{
		_MatchResultWidget->RemoveFromParent();
	}

	_MatchResultWidget = nullptr;
	if (ensureMsgf(_MatchResultWidgetClass && !_MatchResultWidgetClass->HasAnyClassFlags(CLASS_Abstract),
		TEXT("Player controller must specify a Match Result Widget Blueprint.")))
	{
		_MatchResultWidget = CreateWidget<UMatchResultWidget>(this, _MatchResultWidgetClass);
	}

	if (IsValid(_MatchResultWidget))
	{
		_MatchResultWidget->SetResults(Results, ReturnServerTime,
			IsValid(PlayerState) ? PlayerState->GetPlayerId() : INDEX_NONE);

		_MatchResultWidget->AddToViewport(1000);

		const auto* HUD = Cast<AMainHUD>(GetHUD());

		if (!HUD || !HUD->IsExitMenuOpen())
		{
			RestoreMatchResultInput();
		}
	}

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void APlayerControllerBase::ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason)
{
	UGameInstance* Instance = GetGameInstance();

	UFPSOnlineSessionSubsystem* Sessions = Instance ? Instance->GetSubsystem<UFPSOnlineSessionSubsystem>() : nullptr;

	if (IsValid(Sessions))
	{
		Sessions->ReturnToLobby();

		return;
	}

	Super::ClientReturnToMainMenuWithTextReason_Implementation(ReturnReason);
}

void APlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::ToggleExitMenu);
}

void APlayerControllerBase::ToggleExitMenu()
{
	if (true == IsLocalController())
	{
		if (auto* HUD = Cast<AMainHUD>(GetHUD()))
		{
			HUD->ToggleExitMenu();
		}
	}
}

bool APlayerControllerBase::RestoreMatchResultInput()
{
	if (!IsValid(_MatchResultWidget) || !_MatchResultWidget->IsInViewport())
	{
		return false;
	}

	FInputModeUIOnly Mode;

	Mode.SetWidgetToFocus(_MatchResultWidget->TakeWidget());

	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(Mode);

	SetShowMouseCursor(true);

	return true;
}

void APlayerControllerBase::ChangeState(FName NewState)
{
	Super::ChangeState(NewState);

	// 엔진의 Pawn/관전자 전환이 끝난 뒤 입력을 맞춘다.
	RefreshInputMappingContext();
}

void APlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// 로비의 UI 입력 모드 잔재 방비 -> 게임 진입 시 게임 입력 모드로 명시
	if (true == IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}

	RefreshInputMappingContext();
}

void APlayerControllerBase::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateServerTimeSync();
}

void APlayerControllerBase::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	// 유지된 Controller에는 BeginPlay가 다시 호출되지 않는다.
	_PendingServerTimeRequests.Empty();
	_TimeSyncGameState.Reset();
	_ServerTimeSamplesReceived = 0;
	_NextServerTimeRequestAt = 0.0;
	if (IsValid(_MatchResultWidget))
	{
		_MatchResultWidget->RemoveFromParent();
		_MatchResultWidget = nullptr;
	}
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
	RefreshInputMappingContext();
}

void APlayerControllerBase::UpdateServerTimeSync()
{
	if (!IsLocalController() || HasAuthority() || !GetWorld()) return;

	auto* GameState = GetWorld()->GetGameState<AFPSGameState>();

	if (_TimeSyncGameState.Get() != GameState)
	{
		// A new map has a new clock. Never apply responses from the previous world.
		_PendingServerTimeRequests.Empty();

		_TimeSyncGameState = GameState;

		_ServerTimeSamplesReceived = 0;

		_NextServerTimeRequestAt = 0.0;
	}

	if (!GameState)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();

	for (auto It = _PendingServerTimeRequests.CreateIterator(); It; ++It)
	{
		if (Now - It.Value().SentRealTime > 5.0)
		{
			It.RemoveCurrent();
		}
	}

	if (Now < _NextServerTimeRequestAt)
	{
		return;
	}

	const uint32 RequestId = ++_NextServerTimeRequestId;

	FPendingServerTimeRequest& Request = _PendingServerTimeRequests.Add(RequestId);

	Request.SentRealTime = Now;

	Request.SentWorldTime = GetWorld()->GetTimeSeconds();

	Request.GameState = GameState;

	ServerRequestServerTime(RequestId);

	_NextServerTimeRequestAt = Now + (_ServerTimeSamplesReceived < 6 ? 0.25 : 2.0);
}

void APlayerControllerBase::ServerRequestServerTime_Implementation(uint32 RequestId)
{
	if (auto* GameState = GetWorld()->GetGameState<AFPSGameState>())
	{
		ClientReceiveServerTime(RequestId, GetWorld()->GetTimeSeconds(), GameState);
	}
}

void APlayerControllerBase::ClientReceiveServerTime_Implementation(uint32 RequestId, double ServerWorldTime, AFPSGameState* ServerGameState)
{
	FPendingServerTimeRequest Request;

	if (!_PendingServerTimeRequests.RemoveAndCopyValue(RequestId, Request) || !GetWorld()) 
	{
		return;
	}

	if (!IsLocalController() || HasAuthority() || !IsValid(ServerGameState)
		|| ServerGameState != Request.GameState.Get()
		|| ServerGameState != GetWorld()->GetGameState<AFPSGameState>()) 
	{
		return;
	}

	const double RealRoundTrip = FPlatformTime::Seconds() - Request.SentRealTime;

	const double WorldRoundTrip = GetWorld()->GetTimeSeconds() - Request.SentWorldTime;

	if (RealRoundTrip < 0.0 || RealRoundTrip > 5.0 || WorldRoundTrip < 0.0 || !FMath::IsFinite(ServerWorldTime)) 
	{
		return;
	}

	// World seconds keep the estimate in the same units as the match deadlines.
	ServerGameState->ApplyServerTimeSample(ServerWorldTime + WorldRoundTrip * 0.5, RealRoundTrip);

	++_ServerTimeSamplesReceived;
}

void APlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RefreshInputMappingContext();
}

void APlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();

	RefreshInputMappingContext();
}

void APlayerControllerBase::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	RefreshInputMappingContext();
}

void APlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	_PendingServerTimeRequests.Empty();
	
	_TimeSyncGameState.Reset();

	if (IsValid(_MatchResultWidget))
	{
		_MatchResultWidget->RemoveFromParent();

		_MatchResultWidget = nullptr;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();

	if (nullptr != LocalPlayer)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem< UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

		if (true == IsValid(Subsystem))
		{
			if (true == IsValid(_PlayerMappingContext))
			{
				Subsystem->RemoveMappingContext(_PlayerMappingContext.Get());
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void APlayerControllerBase::EnterDeathSpectating(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	if (false == HasAuthority())
	{
		return;
	}

	SetSpawnLocation(CameraLocation);

	SetControlRotation(CameraRotation);

	// 참가자 자격은 유지하고 리스폰을 기다리는 동안만 관전한다.
	if (true == IsValid(PlayerState))
	{
		PlayerState->SetIsSpectator(false);

		PlayerState->SetIsOnlyASpectator(false);
	}

	ChangeState(NAME_Spectating);

	bPlayerIsWaiting = true;

	ClientEnterDeathSpectating(CameraLocation, CameraRotation);
}

void APlayerControllerBase::ClientEnterDeathSpectating_Implementation(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	SetSpawnLocation(CameraLocation);

	SetControlRotation(CameraRotation);

	ChangeState(NAME_Spectating);

	bPlayerIsWaiting = true;
}

void APlayerControllerBase::RefreshInputMappingContext()
{
	if (false == IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();

	if (false == IsValid(LocalPlayer))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (false == IsValid(Subsystem))
	{
		return;
	}

	UInputMappingContext* DesiredContext = nullptr;

	if (NAME_Playing == GetStateName() && true == IsValid(GetPawn()))
	{
		DesiredContext = _PlayerMappingContext.Get();
	}
	if (IsValid(_PlayerMappingContext) && _PlayerMappingContext.Get() != DesiredContext
		&& Subsystem->HasMappingContext(_PlayerMappingContext.Get()))
	{
		Subsystem->RemoveMappingContext(_PlayerMappingContext.Get());
	}

	if (true == IsValid(DesiredContext) && false == Subsystem->HasMappingContext(DesiredContext))
	{
		Subsystem->AddMappingContext(DesiredContext, 0);
	}
}
