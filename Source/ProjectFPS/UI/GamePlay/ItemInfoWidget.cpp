// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/ItemInfoWidget.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"




void UItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 시작은 숨김 처리.
	HideInfo();	
}

void UItemInfoWidget::SetInfoByTID(FName TID)
{
	// 받아 올 행이 없을 경우 숨김처리
	if (TID.IsNone())
	{
		HideInfo();
		return;
	}

	// 테이블 조회용 시스템 가져오기.
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;

	// TID(ItemTable) 해당하는 행 찾기.
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);
	
	// 정보 값을 못 찾을 경우 숨기고 종료.
	if (nullptr == Row)
	{
		HideInfo();
		return;
	}

	// 이름
	if (nullptr != _NameText)
		_NameText->SetText(Row->_DisplayName);

	// 설명
	if (nullptr != _DescText)
		_DescText->SetText(Row->_Description);

	// 아이콘
	if (nullptr != _IconImage)
	{
		if (nullptr != Row->_Icon)
		{
			_IconImage->SetBrushFromTexture(Row->_Icon);
			_IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			_IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UItemInfoWidget::HideInfo()
{
	// *걍 삭제를 할까?
	// ESlateVisibility -> 위젯 표시 상태를 모아 놓은 enum 
	SetVisibility(ESlateVisibility::Hidden);
}
