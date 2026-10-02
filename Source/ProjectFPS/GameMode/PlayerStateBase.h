// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/PlayerMatchStats.h"
#include "PlayerStateBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerMatchStatsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerDeathStateChanged, bool, IsDead);

/**
 * *인벤토리 -> 컴포넌트로 이동예정	
 */
UCLASS()
class PROJECTFPS_API APlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
    APlayerStateBase();
    virtual void SetPlayerName(const FString& Name) override;
    virtual void OnRep_PlayerName() override;
    virtual void OnRep_bIsInactive() override;


public:
	UPROPERTY(BlueprintAssignable, Category = "Score")
	FPlayerMatchStatsChanged _OnMatchStatsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FPlayerDeathStateChanged _OnDeathStateChanged;
private:
	// 인벤 컴포넌트 부착
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UInventoryComponent> _InventoryComponent;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool _IsDead = false;

	UPROPERTY(ReplicatedUsing = OnRep_MatchStats)
	FPlayerMatchStats _MatchStats;
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	//void AddItem(FName TID, int32 Count = 1);
	UInventoryComponent* GetInventory() const 
	{ 
		return _InventoryComponent;
	}
	
public:
	UFUNCTION(BlueprintPure, Category = "Score")
	const FPlayerMatchStats& GetMatchStats() const 
	{ 
		return _MatchStats;
	}

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsDead() const;

protected:
	/**
	 * Dead 변수가 바뀌었을 때 호출되는 이벤트성 멤버함수.
	 */
	UFUNCTION()
	void OnRep_IsDead();

	UFUNCTION()
	void OnRep_MatchStats();

public:
	void AddKillScore();
	
	void AddDeathScore();
	
	void ResetMatchStats();

	void SetDead(bool IsDead);

};
