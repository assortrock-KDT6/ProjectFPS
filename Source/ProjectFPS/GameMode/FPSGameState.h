#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameMode/PlayerMatchStats.h"
#include "FPSGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMatchInformationChanged);

UCLASS()
class PROJECTFPS_API AFPSGameState : public AGameState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FMatchInformationChanged _OnMatchInformationChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	float _MatchDurationSeconds = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	int32 _ReadyPlayerCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	int32 _ExpectedPlayerCount = 1;

	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	bool _StartCountdownActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	double _MatchStartServerTime = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	double _MatchEndServerTime = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchInformation)
	TArray<FPlayerMatchResult> _MatchResults;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void OnRep_MatchState() override;

public:
	UFUNCTION(BlueprintPure, Category = "Match")
	float GetRemainingMatchTime() const;

	UFUNCTION(BlueprintPure, Category = "Match|Start")
	float GetRemainingStartDelay() const;

	UFUNCTION(BlueprintPure, Category = "Match|Start")
	bool IsStartCountdownActive() const 
	{ 
		return _StartCountdownActive;
	}

	UFUNCTION(BlueprintPure, Category = "Match|Start")
	int32 GetReadyPlayerCount() const 
	{ 
		return _ReadyPlayerCount;
	}

	UFUNCTION(BlueprintPure, Category = "Match|Start")
	int32 GetExpectedPlayerCount() const 
	{ 
		return _ExpectedPlayerCount;
	}

	UFUNCTION(BlueprintPure, Category = "Match")
	const TArray<FPlayerMatchResult>& GetMatchResults() const 
	{ 
		return _MatchResults;
	}

public:
	void StartMatchClock(float DurationSeconds);

	void PrepareMatch(float DurationSeconds, int32 ExpectedPlayerCount);

	void UpdateWaitingPlayers(int32 ReadyPlayerCount, int32 ExpectedPlayerCount);

	void StartMatchCountdown(float DelaySeconds);

	void CancelMatchCountdown();

	void RecordMatchResults();

private:
	UFUNCTION()
	void OnRep_MatchInformation();
};
