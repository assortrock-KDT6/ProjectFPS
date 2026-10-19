// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GamePlay/WeaponStatusWidget.h"
#include "Character/CharacterPlayer.h"
#include "Weapons/WeaponActor.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

void UWeaponStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	_BoundPlayerController = GetOwningPlayer();
	if (true == _BoundPlayerController.IsValid())
	{
		_BoundPlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UWeaponStatusWidget::HandlePlayerPawnChanged);
	}

	HandlePlayerPawnChanged(nullptr, GetOwningPlayerPawn());
}

void UWeaponStatusWidget::NativeDestruct()
{
	if (true == _BoundPlayerController.IsValid())
	{
		_BoundPlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UWeaponStatusWidget::HandlePlayerPawnChanged);
	}

	_BoundPlayerController.Reset();
	HandlePlayerPawnChanged(nullptr, nullptr);

	Super::NativeDestruct();
}

void UWeaponStatusWidget::HandlePlayerPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (true == _BoundCharacter.IsValid())
	{
		_BoundCharacter->WeaponEquppedChanged.RemoveDynamic(this, &UWeaponStatusWidget::HandleWeaponEquippedChanged);
	}

	_BoundCharacter = Cast<ACharacterPlayer>(NewPawn);
	if (true == _BoundCharacter.IsValid())
	{
		_BoundCharacter->WeaponEquppedChanged.AddUniqueDynamic(this, &UWeaponStatusWidget::HandleWeaponEquippedChanged);
	}

	BindWeapon(_BoundCharacter.IsValid() ? _BoundCharacter->GetEquippedWeapon() : nullptr);
}

void UWeaponStatusWidget::HandleWeaponEquippedChanged(bool bEquipped)
{
	// 장착 여부가 계속 true여도 교체된 무기 액터를 다시 연결한다.
	BindWeapon(_BoundCharacter.IsValid() ? _BoundCharacter->GetEquippedWeapon() : nullptr);
}

void UWeaponStatusWidget::BindWeapon(AWeaponActor* Weapon)
{
	if (true == _BoundWeapon.IsValid())
	{
		_BoundWeapon->_OnAmmoChanged.RemoveDynamic(this, &UWeaponStatusWidget::UpdateAmmo);
		_BoundWeapon->_OnWeaponDataChanged.RemoveDynamic(this, &UWeaponStatusWidget::UpdateWeaponStatus);
	}

	_BoundWeapon = Weapon;
	if (true == _BoundWeapon.IsValid())
	{
		_BoundWeapon->_OnAmmoChanged.AddUniqueDynamic(this, &UWeaponStatusWidget::UpdateAmmo);
		_BoundWeapon->_OnWeaponDataChanged.AddUniqueDynamic(this, &UWeaponStatusWidget::UpdateWeaponStatus);
	}

	// 이벤트 구독 이전에 정해진 값도 첫 화면에 표시한다.
	UpdateWeaponStatus();
}

void UWeaponStatusWidget::UpdateWeaponStatus()
{
	if (false == _BoundWeapon.IsValid() || false == _BoundWeapon->GetWeaponData().IsValid())
	{
		if (nullptr != _IconImage)
		{
			_IconImage->SetBrushFromTexture(nullptr);
		}

		if (nullptr != _AmmoText)
		{
			_AmmoText->SetText(FText::GetEmpty());
		}

		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UTexture2D* Icon = _BoundWeapon->GetWeaponData()._Icon;
	if (nullptr != _IconImage)
	{
		_IconImage->SetBrushFromTexture(Icon);
		_IconImage->SetVisibility(nullptr != Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	UpdateAmmo(_BoundWeapon->GetCurrentAmmo(), _BoundWeapon->GetMaxAmmo());
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWeaponStatusWidget::UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (nullptr != _AmmoText)
	{
		_AmmoText->SetText(FText::Format(
			NSLOCTEXT("WeaponStatus", "AmmoCount", "{0} | {1}"),
			FText::AsNumber(CurrentAmmo), FText::AsNumber(MaxAmmo)));
	}
}
