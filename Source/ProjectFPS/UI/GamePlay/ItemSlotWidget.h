// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Common/GameDefines.h"
#include "ItemSlotWidget.generated.h"

/**
 * 
 */

// 정보를 전달하기 위한 용도 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHovered, FName, TID);
// 우클릭 버리기 요청
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSlotDropRequested, EItemType, Type, int32, Index, int32, Count);


UCLASS()
class PROJECTFPS_API UItemSlotWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

	// 테두리 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> _Highlight;

	// 아이콘 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> _IconImage;

	// 아이템 수량 
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> _CountText;

	// 아이템 정보 패널
	UPROPERTY()
	TObjectPtr<class UItemInfoWidget> _InfoPanel;

	// 이 슬롯이 표시 중인 아이템의 키 조회
	FName _TID = NAME_None;

	// 이 슬롯이 인벤의 어느 칸인진 
	EItemType _Type = EItemType::None;
	int32 _Index = INDEX_NONE;
	int32 _Count = 0;

public:
	UPROPERTY(BlueprintAssignable)
	FOnSlotDropRequested _OnDropRequested;
	


protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	// 마우스 
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override; 
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;



public:
	void SetSlot(FName TID);
	
	void SetHighlight(bool bOn);	// 쉐이더로 변경해야함 지금은 꼼수상태.	
	
	void SetInfoPanel(class UItemInfoWidget* Panel) { _InfoPanel = Panel; }

	// 고정슬롯(무기) 
	void SetSlotId(EItemType Type, int32 Index) { _Type = Type; _Index = Index; }
	FName GetTID() const { return _TID; }
};
