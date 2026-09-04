// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

/**
 * 
 */

struct FOnAttributeChangeData;

/**
 * 구조계층 바꿀 확률 큽니다.
 * 우선 테스트로 실행해보겠습니다. (09.04)
 */

UCLASS()
class PROJECTFPS_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly,meta = (AssetRegistrySearchable = "true", BindWidget))
	TObjectPtr<class UProgressBar> _Gauge;

	UPROPERTY()
	TObjectPtr<class UAbilitySystemComponent> _AbilitySystemComponent;

protected:
	FDelegateHandle _HealthChangedHandle;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void RefreshHealth(float Health);
public:
	UFUNCTION()
	void SetHpBarUpdate(float percent);
};
