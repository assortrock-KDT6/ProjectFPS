// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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

UCLASS()
class PROJECTFPS_API APlayerControllerBase : public APlayerController
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

protected:
	UPROPERTY(BlueprintReadWrite)
	uint8 _TeamId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> _PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Match|Results")
	TSubclassOf<UMatchResultWidget> _MatchResultWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMatchResultWidget> _MatchResultWidget;

public:
	virtual void ChangeState(FName NewState) override;

	virtual void BeginPlay() override;

	virtual void PostSeamlessTravel() override;

	virtual void PlayerTick(float DeltaTime) override;

	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	virtual bool IsMoveInputIgnored() const override;

	virtual bool IsLookInputIgnored() const override;

	virtual void SetupInputComponent() override;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	virtual void OnRep_Pawn() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason) override;

	void SetLoadingInputBlocked(bool Blocked);

public:
	void EnterDeathSpectating(const FVector& CameraLocation, const FRotator& CameraRotation);

	void RefreshInputMappingContext();

public:
	UFUNCTION(Client, Reliable)
	void ClientShowMatchResults(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime);

	UFUNCTION(BlueprintCallable, Category = "Menu") void ToggleExitMenu();
	bool RestoreMatchResultInput();

protected:
	UFUNCTION(Client, Reliable)
	void ClientEnterDeathSpectating(const FVector& CameraLocation, const FRotator& CameraRotation);

private:
	// 유실된 샘플은 별도로 큐에 보관하지 않고, 다음 요청에서 다시 시도한다.
	UFUNCTION(Server, Unreliable)
	void ServerRequestServerTime(uint32 RequestId);

	UFUNCTION(Client, Unreliable)
	void ClientReceiveServerTime(uint32 RequestId, double ServerWorldTime, AFPSGameState* ServerGameState);

private:
	void UpdateServerTimeSync();

};
