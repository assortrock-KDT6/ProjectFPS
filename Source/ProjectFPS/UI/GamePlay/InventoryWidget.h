// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

/**
 * 
 */


UCLASS()
class PROJECTFPS_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTileView> _ItemTileView; // 소모품.

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UItemSlotWidget> _MainWeaponSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UItemSlotWidget> _SubWeaponSlot;


	
protected:
	virtual void NativeConstruct() override;

public:
	void Refresh();
};
