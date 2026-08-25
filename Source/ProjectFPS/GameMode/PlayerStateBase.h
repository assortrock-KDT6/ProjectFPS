// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PlayerStateBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API APlayerStateBase : public APlayerState
{
	GENERATED_BODY()
	
	// 지워질 예정? (보류)
	// 시작 아이템
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TMap<FName, int32> _StartItems; 

	// 현재 보유 (TID -> 개수)
	UPROPERTY()
	TMap<FName, int32> _Items;

	
public:
	virtual void BeginPlay() override;
	
	void AddItem(FName TID, int32 Count = 1);

public:
	const TMap<FName, int32>& GetItems() const 
	{
		return _Items;
	}

};
