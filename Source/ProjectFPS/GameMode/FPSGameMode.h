#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TimerManager.h"
#include "FPSGameMode.generated.h"

class ACharacterPlayer;
class APlayerStateBase;

/** 제한시간 동안 사망한 참가자가 계속 리스폰하는 개인 데스매치. */
UCLASS()
class PROJECTFPS_API AFPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AFPSGameMode();

private:
	FTimerHandle _MatchStartCheckTimer;

	FTimerHandle _MatchEndTimer;

	FTimerHandle _ReturnToLobbyTimer;

	TMap<TWeakObjectPtr<APlayerController>, FTimerHandle> _RespawnTimers;

public:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void StartMatch() override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	virtual void Logout(AController* Exiting) override;

protected:
	virtual bool ReadyToStartMatch_Implementation() override;

	virtual void HandleMatchIsWaitingToStart() override;

	virtual void HandleMatchHasStarted() override;

	virtual void HandleMatchHasEnded() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsCombatAllowed() const;

	void HandlePlayerDeath(ACharacterPlayer* DeadCharacter, APlayerStateBase* KillerPlayerState);
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0", Units = "s"))
	float _MatchDurationSeconds = 300.f;

	// 서버가 결과창을 보여준 뒤 로비 복귀를 시작하기까지의 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match|Results", meta = (ClampMin = "0.1", UIMin = "0.1", Units = "s", DisplayName = "Result Display Duration Seconds"))
	float _ResultDisplayDurationSeconds = 10.f;

	// 로비 이동 시 전달한 ExpectedPlayers가 우선한다. 직접 실행/PIE에서는 이 값을 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match|Start", meta = (ClampMin = "1"))
	int32 _ExpectedPlayerCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match|Start", meta = (ClampMin = "0.0", Units = "s"))
	float _MatchStartDelaySeconds = 5.f;

private:
	void FinishTimedMatch();

	void ReturnToLobbyAfterResults();

	void UpdateMatchStart();

	void CancelMatchStartCountdown();

	bool CanStartMatch() const;

	bool IsMatchParticipant(const APlayerController* PlayerController) const;

	bool IsPlayerReady(APlayerController* PlayerController) const;

	void CountReadyPlayers(int32& ReadyPlayerCount, int32& ConnectedPlayerCount) const;

	void ScheduleRespawn(APlayerController* PlayerController);

	void RespawnPlayer(TWeakObjectPtr<APlayerController> PlayerController);

	void CancelRespawn(APlayerController* PlayerController);

	void ClearMatchTimers();

	void SetPlayersMatchCombatBlocked(bool Blocked);
};
