// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/ItemSlotWidget.h"
#include "UI/GamePlay/ItemObject.h"
#include "Table/TableSubsystem.h"	// *GameInst로 옮기기 
#include "Table/TableDatas.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"


void UItemSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
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

void UItemSlotWidget::SetSlot(FName TID)
{
	// 빈 슬롯
	if (TID.IsNone())
	{
		if (IconImage)
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		if (CountText)
			CountText->SetText(FText::GetEmpty());
		return;
	}

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
		IconImage->SetVisibility(ESlateVisibility::Visible);
		if (Row->_Icon)
			IconImage->SetBrushFromTexture(Row->_Icon);
	}
}
