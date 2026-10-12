#include "GameMode/FPSGameState.h"
#include "GameMode/PlayerStateBase.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

double AFPSGameState::GetServerWorldTimeSeconds() const
{
	if (HasAuthority() || !_HasServerTimeSync || !GetWorld())
	{
		return Super::GetServerWorldTimeSeconds();
	}

	// Smoothing a new estimate must not make an already displayed clock run backwards.
	_LastSynchronizedServerTime = FMath::Max(_LastSynchronizedServerTime,
		GetWorld()->GetTimeSeconds() + _SynchronizedServerTimeOffset);

	return _LastSynchronizedServerTime;
}

void AFPSGameState::ApplyServerTimeSample(double EstimatedServerTime, double RoundTripSeconds)
{
	if (HasAuthority() || !GetWorld() || !FMath::IsFinite(EstimatedServerTime)
		|| !FMath::IsFinite(RoundTripSeconds) || RoundTripSeconds < 0.0 || RoundTripSeconds > 5.0) return;

	const double Now = FPlatformTime::Seconds();

	_ServerTimeSamples.RemoveAll([Now](const FServerTimeSample& Sample)
	{
		return Now - Sample.ReceivedAt > 10.0;
	});

	if (_ServerTimeSamples.Num() >= 8)
	{
		_ServerTimeSamples.RemoveAt(0);
	}

	_ServerTimeSamples.Add({EstimatedServerTime - GetWorld()->GetTimeSeconds(), RoundTripSeconds, Now});

	// The shortest recent round trip is least affected by queueing and delayed frames.
	const FServerTimeSample* Best = &_ServerTimeSamples[0];

	for (const FServerTimeSample& Sample : _ServerTimeSamples)
	{
		if (Sample.RoundTripSeconds < Best->RoundTripSeconds)
		{
			Best = &Sample;
		}
	}
	if (!_HasServerTimeSync)
	{
		_ServerTimeSyncStartedAt = Now;
	}
	// Acquire the clock quickly during loading; smooth only subsequent maintenance.

	_SynchronizedServerTimeOffset = Now - _ServerTimeSyncStartedAt < 3.0
		? Best->Offset : FMath::Lerp(_SynchronizedServerTimeOffset, Best->Offset, 0.25);

	_HasServerTimeSync = true;
}

void AFPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSGameState, _MatchEndServerTime);

	DOREPLIFETIME(AFPSGameState, _MatchResults);

	DOREPLIFETIME(AFPSGameState, _MatchDurationSeconds);

	DOREPLIFETIME(AFPSGameState, _ReadyPlayerCount);

	DOREPLIFETIME(AFPSGameState, _ExpectedPlayerCount);

	DOREPLIFETIME(AFPSGameState, _StartCountdownActive);

	DOREPLIFETIME(AFPSGameState, _MatchStartServerTime);
}

float AFPSGameState::GetRemainingMatchTime() const
{
	if (true == HasMatchEnded())
	{
		return 0.f;
	}
	if (false == IsMatchInProgress() || _MatchEndServerTime <= 0.0)
	{
		return _MatchDurationSeconds;
	}
	return static_cast<float>(FMath::Max(0.0, _MatchEndServerTime - GetServerWorldTimeSeconds()));
}

float AFPSGameState::GetRemainingStartDelay() const
{
	return ( true == _StartCountdownActive && false == HasMatchStarted())
		? static_cast<float>(FMath::Max(0.0, _MatchStartServerTime - GetServerWorldTimeSeconds())) : 0.f;
}

void AFPSGameState::PrepareMatch(float DurationSeconds, int32 ExpectedPlayerCount)
{
	if (false == HasAuthority())
	{
		return;
	}

	_MatchDurationSeconds = FMath::Max(1.f, DurationSeconds);
	
	_ExpectedPlayerCount = FMath::Max(1, ExpectedPlayerCount);
	
	_ReadyPlayerCount = 0;
	
	_MatchEndServerTime = 0.0;
	
	_MatchStartServerTime = 0.0;
	
	_StartCountdownActive = false;
	
	_MatchResults.Empty();
	
	ForceNetUpdate();
	
	OnRep_MatchInformation();
}

void AFPSGameState::UpdateWaitingPlayers(int32 ReadyPlayerCount, int32 ExpectedPlayerCount)
{
	if (false == HasAuthority() || (ReadyPlayerCount == _ReadyPlayerCount && ExpectedPlayerCount == _ExpectedPlayerCount))
	{
		return;
	}
	_ReadyPlayerCount = ReadyPlayerCount;

	_ExpectedPlayerCount = ExpectedPlayerCount;

	ForceNetUpdate();

	OnRep_MatchInformation();
}

void AFPSGameState::StartMatchCountdown(float DelaySeconds)
{
	if (false == HasAuthority())
	{
		return;
	}
	_StartCountdownActive = true;

	_MatchStartServerTime = GetServerWorldTimeSeconds() + FMath::Max(0.f, DelaySeconds);

	ForceNetUpdate();

	OnRep_MatchInformation();
}

void AFPSGameState::CancelMatchCountdown()
{
	if (false == HasAuthority() || false == _StartCountdownActive)
	{
		return;
	}

	_StartCountdownActive = false;

	_MatchStartServerTime = 0.0;

	ForceNetUpdate();

	OnRep_MatchInformation();
}

void AFPSGameState::MultiCastKillLogged_Implementation(const FPlayerKillLogResult& Result)
{
	_OnKillLogged.Broadcast(Result);
}

void AFPSGameState::StartMatchClock(float DurationSeconds)
{
	if (false == HasAuthority())
	{
		return;
	}

	_MatchDurationSeconds = FMath::Max(1.f, DurationSeconds);

	_MatchEndServerTime = GetServerWorldTimeSeconds() + _MatchDurationSeconds;

	_StartCountdownActive = false;

	_MatchStartServerTime = 0.0;

	_MatchResults.Empty();

	ForceNetUpdate();

	OnRep_MatchInformation();
}

void AFPSGameState::RecordMatchResults()
{
	if (false == HasAuthority())
	{
		return;
	}

	_MatchEndServerTime = GetServerWorldTimeSeconds();

	_MatchResults.Empty();

	for (APlayerState* PlayerState : PlayerArray)
	{
		const APlayerStateBase* FPSPlayerState = Cast<APlayerStateBase>(PlayerState);

		if (false == IsValid(FPSPlayerState) || true == FPSPlayerState->IsOnlyASpectator() || true == FPSPlayerState->IsInactive())
		{
			continue;
		}

		FPlayerMatchResult& Result = _MatchResults.AddDefaulted_GetRef();

		Result._PlayerId = FPSPlayerState->GetPlayerId();

		Result._PlayerName = FPSPlayerState->GetPlayerName();

		Result._Stats = FPSPlayerState->GetMatchStats();
	}

	for (FPlayerMatchResult& Result : _MatchResults)
	{
		Result._Rank = 1;

		for (const FPlayerMatchResult& OtherResult : _MatchResults)
		{
			if (OtherResult._Stats._KillScore > Result._Stats._KillScore)
			{
				++Result._Rank;
			}
		}
	}

	ForceNetUpdate();

	OnRep_MatchInformation();
}

void AFPSGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	OnRep_MatchInformation();
}

void AFPSGameState::OnRep_MatchInformation()
{
	_OnMatchInformationChanged.Broadcast();
}

