// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropCountWidget.generated.h"

/**
 * 
 */

 // 수량 확정 (슬롯 번호, 버릴 개수)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDropCountConfirmed, int32, Index, int32, Count);


UCLASS()
class PROJECTFPS_API UDropCountWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USpinBox> _CountSpinBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> _ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> _CancelButton;

	// 지금 어느 슬롯을 버리는 중인지.
	int32 _TargetIndex = INDEX_NONE;
	int32 _MaxCount = 0;

public:
	UPROPERTY(BlueprintAssignable)
	FOnDropCountConfirmed _OnConfirmed;
	
protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleConfirm();

	UFUNCTION()
	void HandleCancel();
public:
	// 슬롯 번호와 최대 수량으로 열기
	void Open(int32 Index, int32 MaxCount);
	void Close();

};
