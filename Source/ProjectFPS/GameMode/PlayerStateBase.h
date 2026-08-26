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

	
public:
	
	//void AddItem(FName TID, int32 Count = 1);
	UInventoryComponent* GetInventory() const { return _InventoryComponent; }
	

};
