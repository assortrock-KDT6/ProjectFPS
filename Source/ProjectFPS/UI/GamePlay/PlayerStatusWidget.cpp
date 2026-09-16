// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/PlayerStatusWidget.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "AbilitySystemInterface.h"
#include "Character/CharacterPlayer.h"
#include "Components/ProgressBar.h"


void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ACharacter* Character = Cast<ACharacter>(GetOwningPlayerPawn());
	IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Character);

	if (nullptr == AbilityOwner)
	{
		return;
	}

	_AbilitySystemComponent = AbilityOwner->GetAbilitySystemComponent();
	if (nullptr == _AbilitySystemComponent)
	{
		return;
	}

	InitializeGauge(_AbilitySystemComponent);
}

void UPlayerStatusWidget::NativeDestruct()
{
	Super::NativeDestruct();

	RefreshGauge();

	_AbilitySystemComponent = nullptr;
}

void UPlayerStatusWidget::HandleGaugeChanged(const FOnAttributeChangeData& Data)
{
	if (nullptr == _AbilitySystemComponent)
	{
		return;
	}

	float CurrentValue	= _AbilitySystemComponent->GetNumericAttribute(_TargetAttribute);
	float MaxValue		= _AbilitySystemComponent->GetNumericAttribute(_TargetMaxAttribute);

	UpdateGauge(MaxValue > 0.f ? FMath::Clamp(CurrentValue / MaxValue, 0.f, 1.f): 0.f);
}

void UPlayerStatusWidget::UpdateGauge(float Percent)
{
	SetProgressBarUpdate(Percent);
}

void UPlayerStatusWidget::SetProgressBarUpdate(float Percent)
{
	if (nullptr != _Gauge)
	{
		_Gauge->SetPercent(Percent);
	}
}

void UPlayerStatusWidget::InitializeGauge(UAbilitySystemComponent* AbiltySystemComponent)
{
	RefreshGauge();

	if (nullptr == AbiltySystemComponent)
	{
		return;
	}

	_AbilitySystemComponent = AbiltySystemComponent;

	if (true == _TargetAttribute.IsValid() && true == _TargetMaxAttribute.IsValid())
	{
		_GaugeChangedHandle = _AbilitySystemComponent->
							  GetGameplayAttributeValueChangeDelegate(_TargetAttribute).AddUObject(
							  this, &UPlayerStatusWidget::HandleGaugeChanged);

		float CurrentValue = _AbilitySystemComponent->GetNumericAttribute(_TargetAttribute);
		float MaxValue = _AbilitySystemComponent->GetNumericAttribute(_TargetMaxAttribute);

		_OnGaugeChanged.Broadcast(CurrentValue, MaxValue);

		/**
		 * 만약 성장요소가 추가된다면 MaxVaule도 추가해줘야함. 
		 */
	}
}

void UPlayerStatusWidget::RefreshGauge()
{
	if (nullptr != _AbilitySystemComponent)
	{
		if (true == _TargetAttribute.IsValid())
		{
			_AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(_TargetAttribute).Remove(_GaugeChangedHandle);
		}
	}

	_GaugeChangedHandle.Reset();
}
