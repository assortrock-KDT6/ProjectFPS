// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

/**
 * 
 */

class UItemSlotWidget;

UCLASS()
class PROJECTFPS_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTileView> _ItemTileView; // 소모품.


	// 장비 슬롯 2개로 분리
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UItemSlotWidget> _MainWeaponSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UItemSlotWidget> _SubWeaponSlot;


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UItemInfoWidget> _ItemInfoPanel;

	// Todo 수류탄 및 방어구 슬롯 추가? 고민중

	
protected:
	virtual void NativeConstruct() override;
	void BindSlot(UItemSlotWidget* InSlot);
	void HandleEntryGenerated(UUserWidget& EntryWidget);
	

public:
	void Refresh();
};
