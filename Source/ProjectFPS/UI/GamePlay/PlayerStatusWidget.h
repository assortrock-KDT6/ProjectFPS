// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly,meta = (AssetRegistrySearchable = "true", BindWidget))
	TObjectPtr<class UProgressBar> _HPBar;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

public:
	UFUNCTION()
	void SetHpBarUpdate(float percent);
};
