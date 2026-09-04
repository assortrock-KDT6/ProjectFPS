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
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UPlayerStatusWidget> _HpWidget;

protected:
	virtual void NativeConstruct() override;

};
