// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponStatusWidget.generated.h"

class ACharacterPlayer;
class AWeaponActor;
class UImage;
class UTextBlock;

// 로컬 플레이어가 장착한 무기의 아이콘과 현재 탄약 | 탄창 용량을 표시한다.
UCLASS()
class PROJECTFPS_API UWeaponStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯 BP에 같은 이름의 Image와 TextBlock을 배치한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _AmmoText;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	TWeakObjectPtr<APlayerController> _BoundPlayerController;
	TWeakObjectPtr<ACharacterPlayer> _BoundCharacter;
	TWeakObjectPtr<AWeaponActor> _BoundWeapon;

	UFUNCTION()
	void HandlePlayerPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleWeaponEquippedChanged(bool bEquipped);

	UFUNCTION()
	void UpdateWeaponStatus();

	UFUNCTION()
	void UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo);

	void BindWeapon(AWeaponActor* Weapon);
};
