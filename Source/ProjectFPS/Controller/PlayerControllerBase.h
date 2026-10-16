// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controller/CommonPlayerController.h"
#include "GameMode/PlayerMatchStats.h"
#include "PlayerControllerBase.generated.h"

 /*
  * 
  *	[ 플레이어 컨트롤러 ]
  * 
  *	1) 플레이어 컨트롤러는 플레이어에 빙의하는 존재이다.
  *	2) TeamID를 가지고 있으며, TeamID에 따라 적인지 아군인지 판단한다.
  * 
  * 
  *	[캐릭터 컴포넌트 구조]
  *
  * 1) AbilityComponent		: 캐릭터의 능력치를 관리한다.
  *	2) SkillComponent		: 캐릭터의 스킬을 관리한다. (증강 시스템 / 상인에게 파밍)
  *	3) InventoryComponent	: Item과 Coin이 들어간다.	--> 생각점 : Coin은 Item에 속하는 존재인가?
  *													--> Inventory UI에 시각적으로 보이는 아이템이 아니므로 다른 변수로 나눈다.
  *	3-1) 증강은 상인에게서 코인으로 구매 가능한 아이템이다.
  *	4) 플레이어가 끝까지 들고 있어야하는 데이터는 Controller 쪽으로 옮긴다.	--> Character는 죽으면 Destroy 되지만
  *																		Controller는 플레이어 (조작자)가 살아있으면 계속 유지된다.
  *
  */

class UInputMappingContext;
class UMatchResultWidget;
class AFPSGameState;
class ACharacterPlayer;
class AFPSDeathCameraActor;

UCLASS()
class PROJECTFPS_API APlayerControllerBase : public ACommonPlayerController
{
	GENERATED_BODY()

public:
	APlayerControllerBase();

private:
	struct FPendingServerTimeRequest
	{
		double SentRealTime = 0.0;
		double SentWorldTime = 0.0;
		TWeakObjectPtr<AFPSGameState> GameState;
	};

	TMap<uint32, FPendingServerTimeRequest> _PendingServerTimeRequests;

	TWeakObjectPtr<AFPSGameState> _TimeSyncGameState;

	uint32 _NextServerTimeRequestId = 0;

	int32 _ServerTimeSamplesReceived = 0;

	double _NextServerTimeRequestAt = 0.0;

	// 빙의 이후 ClientRestart에서 초기화되는 엔진 입력 스택과는 별도로 관리한다.	
	bool _LoadingInputBlocked = false;

	bool _DeathCameraActive = false;
	TWeakObjectPtr<ACharacterPlayer> _DeathCharacter;

protected:
	UPROPERTY(BlueprintReadWrite)
	uint8 _TeamId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> _PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Match|Results")
	TSubclassOf<UMatchResultWidget> _MatchResultWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMatchResultWidget> _MatchResultWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Death Camera")
	TSubclassOf<AFPSDeathCameraActor> _DeathCameraClass;

	UPROPERTY(Transient)
	TObjectPtr<AFPSDeathCameraActor> _DeathCamera;

public:
	virtual void ChangeState(FName NewState) override;

	virtual void BeginPlay() override;

	virtual void PostSeamlessTravel() override;

	virtual void PlayerTick(float DeltaTime) override;

	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	virtual bool IsMoveInputIgnored() const override;

	virtual bool IsLookInputIgnored() const override;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	virtual void OnRep_Pawn() override;

	virtual void ClientRestart_Implementation(APawn* NewPawn) override;

	virtual void ServerAcknowledgePossession_Implementation(APawn* InPawn) override;

	virtual void AutoManageActiveCameraTarget(AActor* SuggestedTarget) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason) override;

	void SetLoadingInputBlocked(bool Blocked);

public:
	void EnterDeathSpectating(ACharacterPlayer* DeadCharacter, double DeathEndServerTime,
		const FVector& CameraLocation, const FRotator& CameraRotation);

	void RefreshInputMappingContext();

public:
	// Commit 건영 : 총알 피해 수치를 UMG로 띄우기
	// UMG를 PlayerControllerBase에서 하는 이유는 피해숫자가 '공격한 플레이어 본인'에게만 보내기 위해서
	// PlayerController는 플레이어 한 명의 입력과 네트워크 연결을 담당하는 객체인데 서버에서 특정 플레이어의 Controller에 Client 함수를 호출하면, 그 플레이어 쪽에서 실행
	// 또한 Controller는 캐릭터가 죽고 다시 생성되는 리스폰 과정에서도 보통 유지돼서, 플레이어에게 보내는 화면 알림을 두기에 적합함
	
	// 서버에서 확정한 피해 정보를 공격자 본인에게 전달
	UFUNCTION(Client, Reliable)
	void ClientShowDamageNumber(FVector WorldLocation, float ShieldDamage, float HealthDamage, bool bHeadShot, AActor* Target, FVector HitLocation);
	
	// 전달 받은 정보를 사용하는 위젯 연출은 Blueprint에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "UI | Damage")
	void ShowDamageNumber(FVector WorldLocation, float ShieldDamage, float HealthDamage, bool bHeadShot, AActor* Target, FVector HitLocation);
	
	UFUNCTION(Client, Reliable)
	void ClientShowMatchResults(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime);

	bool RestoreMatchResultInput();

protected:
	UFUNCTION(Client, Reliable)
	void ClientEnterDeathSpectating(ACharacterPlayer* DeadCharacter, double DeathEndServerTime,
		const FVector& CameraLocation, const FRotator& CameraRotation);

private:
	// 유실된 샘플은 별도로 큐에 보관하지 않고, 다음 요청에서 다시 시도한다.
	UFUNCTION(Server, Unreliable)
	void ServerRequestServerTime(uint32 RequestId);

	UFUNCTION(Client, Unreliable)
	void ClientReceiveServerTime(uint32 RequestId, double ServerWorldTime, AFPSGameState* ServerGameState);

private:
	void UpdateServerTimeSync();
	void TryFinishDeathCamera();
	void ClearDeathCamera(bool RestoreViewTarget);

};
