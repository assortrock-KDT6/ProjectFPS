// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/GameDefines.h"
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

	// 수량 입력 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UDropCountWidget> _DropCountPanel;

protected:
	// 슬롯 우클릭 수신.
	UFUNCTION()
	void HandleSlotDropRequested(EItemType Type, int32 Index, int32 Count);

	// 선택한 수량 받기
	UFUNCTION()
	void HandleDropCountConfirmed(int32 Index, int32 Count);

	// 서버에 버리기 요청
	void RequestDrop(EItemType Type, int32 Index, int32 Count);

	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	void BindSlot(UItemSlotWidget* InSlot);
	void HandleEntryGenerated(UUserWidget& EntryWidget);
	
	// 인벤 변경 알림 수신 -> 다시 그리기
	UFUNCTION()
	void HandleInventoryChanged();
private:
	// pc -> ps -> 인벤 없으면 nullptr
	class UInventoryComponent* FindInventory() const;

public:
	void Refresh();
};
