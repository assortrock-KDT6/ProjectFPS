// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/ItemSlotWidget.h"
#include "UI/GamePlay/ItemObject.h"
#include "Table/TableSubsystem.h"	// *GameInst로 옮기기 
#include "Table/TableDatas.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"


void UItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	
	SetHighlight(false);	
}

void UItemSlotWidget::SetHighlight(bool bOn)
{
	if (Highlight == nullptr)
		return;

	if(bOn)
	{
		Highlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Highlight->SetVisibility(ESlateVisibility::Hidden);
	}

}

void UItemSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// 슬롯 위젯에 아이템 정보를 연결할 때 갱신함.

	UItemObject* Obj = Cast<UItemObject>(ListItemObject);
	if (nullptr == Obj)
		return;

	SetSlot(Obj->_TID);

	// 소모품일 경우 수량만 
	if (CountText)
	{
		if (Obj->_TID.IsNone())
			CountText->SetText(FText::GetEmpty());
		else
			CountText->SetText(FText::AsNumber(Obj->_Count));
	}
}

void UItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	// 빈 슬롯은 무시
	if (_TID.IsNone())
		return;

	SetHighlight(true);
	// 델리게이트에 연결된 함수를 실행하여 값을 전달하는 함수 ->Broadcast
	// 마우스가 올라가있는 슬롯을 알려줌.
	_OnSlotHovered.Broadcast(_TID);
}

void UItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	SetHighlight(false);
	_OnSlotUnHovered.Broadcast(_TID);

}

void UItemSlotWidget::SetSlot(FName TID)
{
	_TID = TID;

	// 아이콘을 숨김		
	if (IconImage)
		IconImage->SetVisibility(ESlateVisibility::Hidden);

	// 수량을 비어져있는 상태로 덮어버림.
	if (CountText)
		CountText->SetText(FText::GetEmpty());

	// 이전 정보를 지움.
	if (TID.IsNone())
		return;

	// 테이블에서 직접 조회 (*헬퍼 예정)
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;

	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);
	if (nullptr == Row)
	{
		if (IconImage)
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// 아이템 있음 
	if (IconImage)
	{
		// 아이콘 위젯을 화면에 표시함.
		IconImage->SetVisibility(ESlateVisibility::Visible);

		
		if (Row->_Icon)
			IconImage->SetBrushFromTexture(Row->_Icon);
	}
}


