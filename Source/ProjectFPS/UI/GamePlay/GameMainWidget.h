// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMainWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UGameMainWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UItemInfoWidget> _ItemInfoPanel;

public:
	void ShowItemInfo(FName TID);
	void HideItemInfo();
	


protected:
	virtual void NativeConstruct() override;

};
