// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/PlayerStatusWidget.h"
#include "Components/ProgressBar.h"

void UPlayerStatusWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

}

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

}

void UPlayerStatusWidget::SetHpBarUpdate(float percent)
{
	if (nullptr != _HPBar)
	{
		_HPBar->SetPercent(percent);
	}
}
