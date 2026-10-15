// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrenadeStatusWidget.generated.h"

/**
 * 로컬 플레이어가 가진 수류탄의 아이콘과 개수를 표시한다. - 수류탄이 없으면 숨긴다.
 * 인벤토리 변경 신호를 받을 때마다 개수를 다시 센다.
 */

class UImage;
class UTextBlock;
class UInventoryComponent;


UCLASS()
class PROJECTFPS_API UGrenadeStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	// 수류탄 아이콘
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _IconImage;

	// 보유 개수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _CountText;

private:
	// 구독중인 인벤토리
	TWeakObjectPtr<UInventoryComponent> _BoundInventory;

	// PlayerState 복제 전이면 인벤토리를 찾을 때까지 재시도
	FTimerHandle _BindRetryHandle;

	// 재시도 간격(초)
	float _BindRetryInterval = 0.2f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void TryBindInventory();
	void UnBindInventory();

	// 인벤토리 변경 신호 수신 -> 개수 다시 계산
	UFUNCTION()
	void UpdateGrenadeStatus();


	
};
