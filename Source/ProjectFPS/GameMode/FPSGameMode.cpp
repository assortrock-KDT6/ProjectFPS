#include "GameMode/FPSGameMode.h"
#include "GameMode/PlayerStateBase.h"
#include "GameMode/FPSGameState.h"
#include "Pawn/FPSSpectatorPawn.h"
#include "Character/CharacterPlayer.h"
#include "Controller/PlayerControllerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"

AFPSGameMode::AFPSGameMode()
{
	GameStateClass = AFPSGameState::StaticClass();
	PlayerStateClass = APlayerStateBase::StaticClass();
	PlayerControllerClass = APlayerControllerBase::StaticClass();
	DefaultPawnClass = ACharacterPlayer::StaticClass();
	SpectatorClass = AFPSSpectatorPawn::StaticClass();
}

void AFPSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	_ExpectedPlayerCount = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("ExpectedPlayers"), _ExpectedPlayerCount));
}

void AFPSGameMode::HandleMatchIsWaitingToStart()
{
	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();
	if (true == IsValid(FPSGameState))
	{
		FPSGameState->PrepareMatch(_MatchDurationSeconds, _ExpectedPlayerCount);
	}

	Super::HandleMatchIsWaitingToStart();
	
	SetPlayersMatchCombatBlocked(true);
	
	GetWorldTimerManager().SetTimer(_MatchStartCheckTimer, this, &AFPSGameMode::UpdateMatchStart, 0.1f, true);
}

bool AFPSGameMode::IsMatchParticipant(const APlayerController* PlayerController) const
{
	const APlayerStateBase* PlayerState = IsValid(PlayerController) ? PlayerController->GetPlayerState<APlayerStateBase>() : nullptr;
	
	return true == IsValid(PlayerState) && false == PlayerController->IsPendingKillPending()
		&& false == PlayerState->IsOnlyASpectator() && false == PlayerState->IsInactive();
}

bool AFPSGameMode::IsPlayerReady(APlayerController* PlayerController) const
{
	if (false == IsMatchParticipant(PlayerController) || false == PlayerController->HasClientLoadedCurrentWorld()
		|| false == IsValid(Cast<ACharacterPlayer>(PlayerController->GetPawn())))
	{
		return false;
	}
	// 원격 플레이어는 접속뿐 아니라 현재 Pawn의 클라이언트 빙의 확인까지 기다린다.
	return PlayerController->IsLocalController() || PlayerController->AcknowledgedPawn == PlayerController->GetPawn();
}

void AFPSGameMode::CountReadyPlayers(int32& ReadyPlayerCount, int32& ConnectedPlayerCount) const
{
	ReadyPlayerCount = 0;
	ConnectedPlayerCount = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (true == IsMatchParticipant(PlayerController))
		{
			++ConnectedPlayerCount;
			if (true == IsPlayerReady(PlayerController))
			{
				++ReadyPlayerCount;
			}
		}
	}
}

void AFPSGameMode::UpdateMatchStart()
{
	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();
	if (false == HasAuthority() || GetMatchState() != MatchState::WaitingToStart || false == IsValid(FPSGameState))
	{
		return;
	}

	// 첫 스폰은 경기 시작 전에 허용한다. 로딩/시작점 충돌 때문에 실패하면 다시 시도한다.
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();

		if (true == IsMatchParticipant(PlayerController) && true == PlayerController->HasClientLoadedCurrentWorld()
			&& false == IsValid(PlayerController->GetPawn()))
		{
			RestartPlayer(PlayerController);
		}
	}

	int32 ReadyPlayerCount;
	int32 ConnectedPlayerCount;
	CountReadyPlayers(ReadyPlayerCount, ConnectedPlayerCount);

	const int32 RequiredPlayerCount = FMath::Max(_ExpectedPlayerCount, ConnectedPlayerCount);
	if (ReadyPlayerCount < RequiredPlayerCount || NumTravellingPlayers > 0
		|| FPSGameState->GetExpectedPlayerCount() != RequiredPlayerCount)
	{
		CancelMatchStartCountdown();
	}

	FPSGameState->UpdateWaitingPlayers(ReadyPlayerCount, RequiredPlayerCount);
	if (ReadyPlayerCount == RequiredPlayerCount && NumTravellingPlayers == 0 && !FPSGameState->IsStartCountdownActive())
	{
		FPSGameState->StartMatchCountdown(_MatchStartDelaySeconds);
	}
	if (true == CanStartMatch())
	{
		StartMatch();
	}
}

bool AFPSGameMode::CanStartMatch() const
{
	const AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();
	if (GetMatchState() != MatchState::WaitingToStart || !IsValid(FPSGameState)
		|| !FPSGameState->IsStartCountdownActive() || FPSGameState->GetRemainingStartDelay() > 0.f || NumTravellingPlayers > 0)
	{
		return false;
	}

	int32 ReadyPlayerCount;
	int32 ConnectedPlayerCount;
	CountReadyPlayers(ReadyPlayerCount, ConnectedPlayerCount);

	return ReadyPlayerCount >= _ExpectedPlayerCount && ReadyPlayerCount == ConnectedPlayerCount
		&& ConnectedPlayerCount == FPSGameState->GetExpectedPlayerCount();
}

bool AFPSGameMode::ReadyToStartMatch_Implementation()
{
	return CanStartMatch();
}

void AFPSGameMode::StartMatch()
{
	// BP나 엔진의 직접 StartMatch 호출도 동일한 준비 조건을 통과해야 한다.
	if (true == HasAuthority() && true == CanStartMatch())
	{
		Super::StartMatch();
	}
}

void AFPSGameMode::CancelMatchStartCountdown()
{
	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();

	if (GetMatchState() == MatchState::WaitingToStart && true == IsValid(FPSGameState))
	{
		FPSGameState->CancelMatchCountdown();
	}
}

void AFPSGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (true == IsMatchParticipant(NewPlayer))
	{
		CancelMatchStartCountdown();
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	if (GetMatchState() == MatchState::WaitingToStart && IsMatchParticipant(NewPlayer))
	{
		RestartPlayer(NewPlayer);
	}
}

void AFPSGameMode::HandleMatchHasStarted()
{
	ClearMatchTimers();

	const float MatchDuration = FMath::Max(1.f, _MatchDurationSeconds);

	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();

	if (true == IsValid(FPSGameState))
	{
		FPSGameState->StartMatchClock(MatchDuration);
		for (APlayerState* PlayerState : FPSGameState->PlayerArray)
		{
			APlayerStateBase* FPSPlayerState = Cast<APlayerStateBase>(PlayerState);
			if (true == IsValid(FPSPlayerState))
			{
				FPSPlayerState->ResetMatchStats();
				FPSPlayerState->SetDead(false);
			}
		}
	}

	// Super에서 최초 Pawn을 생성하므로 먼저 경기 시계를 준비한다.
	Super::HandleMatchHasStarted();
	SetPlayersMatchCombatBlocked(false);
	GetWorldTimerManager().SetTimer(_MatchEndTimer, this, &AFPSGameMode::FinishTimedMatch, MatchDuration, false);
}

bool AFPSGameMode::IsCombatAllowed() const
{
	const AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();

	return IsMatchInProgress() && IsValid(FPSGameState) && FPSGameState->GetRemainingMatchTime() > 0.f;
}

void AFPSGameMode::HandlePlayerDeath(ACharacterPlayer* DeadCharacter, APlayerStateBase* KillerPlayerState)
{
	if (false == HasAuthority() || false == IsCombatAllowed() || false == IsValid(DeadCharacter))
	{
		return;
	}

	APlayerControllerBase* PlayerController = Cast<APlayerControllerBase>(DeadCharacter->GetController());
	APlayerStateBase* DeadPlayerState = DeadCharacter->GetPlayerState<APlayerStateBase>();
	if (false == IsValid(PlayerController) || false == IsValid(DeadPlayerState) || DeadPlayerState->IsDead())
	{
		return;
	}

	// 빙의를 해제하면 Pawn의 PlayerState가 비워진다. 점수는 그 전에 확정한다.
	DeadPlayerState->SetDead(true);
	DeadPlayerState->AddDeathScore();
	if (true == IsValid(KillerPlayerState) && KillerPlayerState != DeadPlayerState
		&& !KillerPlayerState->IsOnlyASpectator() && !KillerPlayerState->IsInactive()
		&& GameState->PlayerArray.Contains(KillerPlayerState))
	{
		KillerPlayerState->AddKillScore();
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	DeadCharacter->StopCombat();

	DeadCharacter->GetCharacterMovement()->StopMovementImmediately();

	DeadCharacter->GetCharacterMovement()->DisableMovement();

	DeadCharacter->SetActorEnableCollision(false);

	DeadCharacter->DetachFromControllerPendingDestroy();

	PlayerController->EnterDeathSpectating(CameraLocation, CameraRotation);

	DeadCharacter->SetLifeSpan(5.f);

	ScheduleRespawn(PlayerController);
}

bool AFPSGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		return IsMatchParticipant(Player) && !IsValid(Player->GetPawn()) && Player->HasClientLoadedCurrentWorld()
			&& AGameModeBase::PlayerCanRestart_Implementation(Player);
	}

	return IsCombatAllowed() && !_RespawnTimers.Contains(Player) && Super::PlayerCanRestart_Implementation(Player);
}

void AFPSGameMode::RestartPlayer(AController* NewPlayer)
{
	APlayerController* PlayerController = Cast<APlayerController>(NewPlayer);

	if (false == HasAuthority() || false == IsValid(PlayerController) || false == PlayerCanRestart(PlayerController))
	{
		return;
	}

	APlayerStateBase* PlayerState = PlayerController->GetPlayerState<APlayerStateBase>();

	const bool WasDead = IsValid(PlayerState) && PlayerState->IsDead();

	Super::RestartPlayer(NewPlayer);

	ACharacterPlayer* Character = Cast<ACharacterPlayer>(NewPlayer->GetPawn());

	if (true == IsValid(Character) && true == IsValid(PlayerState))
	{
		PlayerState->SetDead(false);
		if (WasDead)
		{
			Character->InitializeAfterRespawn();
		}
	}
	else if (false == IsValid(NewPlayer->GetPawn()))
	{
		ScheduleRespawn(PlayerController);
	}
}

void AFPSGameMode::ScheduleRespawn(APlayerController* PlayerController)
{
	if (false == IsCombatAllowed() || false == IsValid(PlayerController) || true == _RespawnTimers.Contains(PlayerController))
	{
		return;
	}

	const TWeakObjectPtr<APlayerController> WeakController(PlayerController);

	FTimerHandle& RespawnTimer = _RespawnTimers.Add(WeakController);

	const FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &AFPSGameMode::RespawnPlayer, WeakController);

	// 0초 타이머는 예약이 해제되므로 최소 한 번의 지연을 보장한다.
	GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, FMath::Max(0.01f, MinRespawnDelay), false);
}

void AFPSGameMode::RespawnPlayer(TWeakObjectPtr<APlayerController> PlayerController)
{
	_RespawnTimers.Remove(PlayerController);

	APlayerController* Controller = PlayerController.Get();

	if (false == IsCombatAllowed() || false == IsValid(Controller) || true == IsValid(Controller->GetPawn())
		|| false == IsValid(Controller->PlayerState) || true == Controller->PlayerState->IsOnlyASpectator())
	{
		return;
	}

	if (false == PlayerCanRestart(Controller))
	{
		ScheduleRespawn(Controller);
		return;
	}

	// 현재 비어 있는 시작점을 다시 선택한다.
	Controller->StartSpot.Reset();
	RestartPlayer(Controller);
}

void AFPSGameMode::CancelRespawn(APlayerController* PlayerController)
{
	FTimerHandle* RespawnTimer = _RespawnTimers.Find(PlayerController);
	if (nullptr != RespawnTimer)
	{
		GetWorldTimerManager().ClearTimer(*RespawnTimer);
		_RespawnTimers.Remove(PlayerController);
	}
}

void AFPSGameMode::Logout(AController* Exiting)
{
	if (true == IsMatchParticipant(Cast<APlayerController>(Exiting)))
	{
		// 예상 인원은 자동 축소하지 않는다. 이탈하면 전원 도착 조건부터 다시 기다린다.
		CancelMatchStartCountdown();
	}

	CancelRespawn(Cast<APlayerController>(Exiting));

	Super::Logout(Exiting);
}

void AFPSGameMode::FinishTimedMatch()
{
	if (true == HasAuthority() && true == IsMatchInProgress())
	{
		EndMatch();
	}
}

void AFPSGameMode::HandleMatchHasEnded()
{
	ClearMatchTimers();

	SetPlayersMatchCombatBlocked(true);

	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();

	if (true == IsValid(FPSGameState))
	{
		FPSGameState->RecordMatchResults();
	}

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();

		ACharacterPlayer* Character = IsValid(PlayerController) ? Cast<ACharacterPlayer>(PlayerController->GetPawn()) : nullptr;

		if (true == IsValid(Character))
		{
			Character->StopCombat();

			Character->GetCharacterMovement()->StopMovementImmediately();

			Character->GetCharacterMovement()->DisableMovement();
		}
	}
	Super::HandleMatchHasEnded();
}

void AFPSGameMode::SetPlayersMatchCombatBlocked(bool Blocked)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* Controller = Iterator->Get();

		const ACharacterPlayer* Character = IsValid(Controller) ? Cast<ACharacterPlayer>(Controller->GetPawn()) : nullptr;

		UFPSAbilitySystemComponent* AbilitySystem = true == IsValid(Character)
			? Cast<UFPSAbilitySystemComponent>(Character->GetAbilitySystemComponent()) : nullptr;
		if (true == IsValid(AbilitySystem))
		{
			AbilitySystem->SetMatchCombatBlocked(Blocked);
		}
	}
}

void AFPSGameMode::ClearMatchTimers()
{
	GetWorldTimerManager().ClearTimer(_MatchStartCheckTimer);
	GetWorldTimerManager().ClearTimer(_MatchEndTimer);
	for (TPair<TWeakObjectPtr<APlayerController>, FTimerHandle>& Entry : _RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Entry.Value);
	}
	_RespawnTimers.Empty();
}

void AFPSGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMatchTimers();
	Super::EndPlay(EndPlayReason);
}
