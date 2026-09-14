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

	_HealthChangedHandle = _AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFPSHealthSet::Get_HealthAttribute()).AddUObject(this, &UPlayerStatusWidget::HandleHealthChanged);

	RefreshHealth(_AbilitySystemComponent->GetNumericAttribute(UFPSHealthSet::Get_HealthAttribute()));
}

void UPlayerStatusWidget::NativeDestruct()
{
	if (nullptr != _AbilitySystemComponent && true == _HealthChangedHandle.IsValid())
	{
		_AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFPSHealthSet::Get_HealthAttribute()).Remove(_HealthChangedHandle);
	}

	_AbilitySystemComponent = nullptr;
	Super::NativeDestruct();
}

void UPlayerStatusWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth(Data.NewValue);
}

void UPlayerStatusWidget::RefreshHealth(float Health)
{
	if (nullptr == _AbilitySystemComponent)
	{
		return;
	}

	const float MaxHealth = _AbilitySystemComponent->GetNumericAttribute(UFPSHealthSet::Get_MaxHealthAttribute());
	SetHpBarUpdate(MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f);
}

void UPlayerStatusWidget::SetHpBarUpdate(float percent)
{
	if (nullptr != _Gauge)
	{
		_Gauge->SetPercent(percent);
	}
}
