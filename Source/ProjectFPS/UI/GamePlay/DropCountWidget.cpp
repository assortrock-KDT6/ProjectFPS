// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/DropCountWidget.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"


void UDropCountWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (_ConfirmButton)
	{
		_ConfirmButton->OnClicked.AddUniqueDynamic(this, &UDropCountWidget::HandleConfirm);
	}

	if (_CancelButton)
	{
		_CancelButton->OnClicked.AddUniqueDynamic(this, &UDropCountWidget::HandleCancel);
	}
	
	Close();
}
void UDropCountWidget::Open(int32 Index, int32 MaxCount)	// N번 슬롯의 N개중 버릴 아이템 수량 선택.
{
	_TargetIndex = Index;
	_MaxCount = MaxCount;

	if (_CountSpinBox)
	{
		_CountSpinBox->SetMinValue(1);
		_CountSpinBox->SetMaxValue(MaxCount);
		_CountSpinBox->SetMinSliderValue(1);
		_CountSpinBox->SetMaxSliderValue(MaxCount);
		_CountSpinBox->SetValue(MaxCount);
	}
	SetVisibility(ESlateVisibility::Visible);
}

void UDropCountWidget::Close()
{
	_TargetIndex = INDEX_NONE;
	_MaxCount = 0;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDropCountWidget::HandleConfirm()
{
	if (INDEX_NONE == _TargetIndex || nullptr == _CountSpinBox)
	{
		Close();
		return;
	}

	int32 Count = FMath::RoundToInt(_CountSpinBox->GetValue());
	Count = FMath::Clamp(Count, 1, _MaxCount);

	const int32 Index = _TargetIndex;
	Close();
	_OnConfirmed.Broadcast(Index, Count);
}

void UDropCountWidget::HandleCancel()
{
	Close();
}

