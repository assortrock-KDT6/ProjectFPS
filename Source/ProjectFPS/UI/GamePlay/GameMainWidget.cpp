// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/GameMainWidget.h"
#include "UI/GamePlay/ItemInfoWidget.h"


void UGameMainWidget::ShowItemInfo(FName TID)
{
	if (nullptr == _ItemInfoPanel)
		return;

	
	_ItemInfoPanel->SetInfoByTID(TID);
	
}

void UGameMainWidget::HideItemInfo()
{
	if (nullptr == _ItemInfoPanel)
		return;

	_ItemInfoPanel->HideInfo();
}

void UGameMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

}
