// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ItemSlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UItemSlotWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

	//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> IconImage;

	//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> CountText;
	


	// 함수 선언
protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
public:
	void SetSlot(FName TID);
};
