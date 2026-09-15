// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PlayerStateBase.generated.h"

/**
 * *인벤토리 -> 컴포넌트로 이동예정	
 */
UCLASS()
class PROJECTFPS_API APlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
	APlayerStateBase();

private:

	// 인벤 컴포넌트 부착
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UInventoryComponent> _InventoryComponent;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool _bIsDead = false;
	

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	//void AddItem(FName TID, int32 Count = 1);
	UInventoryComponent* GetInventory() const { return _InventoryComponent; }
	
public:
	bool IsDead() const;
	void SetDead(const bool bIsDead);

protected:
	/**
	 * Dead 변수가 바뀌었을 때 호출되는 이벤트성 멤버함수. 
	 */
	UFUNCTION()
	void OnRep_IsDead() {};
};
