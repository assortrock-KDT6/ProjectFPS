// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/ItemSlotWidget.h"
#include "UI/GamePlay/ItemObject.h"
#include "Table/TableSubsystem.h"		// *GameInst로 옮기기 
#include "Components/Image.h"
#include "Components/TextBlock.h"


void UItemSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	UItemObject* Obj = Cast<UItemObject>(ListItemObject);
	if (nullptr == Obj)
		return;

	// 개수 = 보유 데이터
	if (CountText)
		CountText->SetText(FText::AsNumber(Obj->_Count));

	// 아이콘 = 테이블에서 TID로 조회
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;

	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), Obj->_TID);
	if (Row && IconImage && Row->_Icon)
		IconImage->SetBrushFromTexture(Row->_Icon);
}
