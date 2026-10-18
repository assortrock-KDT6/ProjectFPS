// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrenadeStatusWidget.generated.h"

/**
 * 로컬 플레이어가 고른 투척물의 아이콘과 개수를 표시한다. - 투척물이 없으면 숨긴다.
 * 인벤토리 변경, 투척물 종류 변경 신호를 받을 때마다 다시 계산한다.
 */

class UImage;
class UTextBlock;
class UInventoryComponent;
class ACharacterPlayer;
class APawn;


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

	// 투척물 이름 (아이템 테이블의 _DisplayName). 위젯에 없으면 표시하지 않는다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> _HeadingText;

private:
	// 구독중인 인벤토리
	TWeakObjectPtr<UInventoryComponent> _BoundInventory;

	// 고른 투척물 종류를 알려주는 캐릭터 (리스폰하면 바뀐다)
	TWeakObjectPtr<ACharacterPlayer> _BoundCharacter;

	// 폰 변경 알림을 받는 컨트롤러
	TWeakObjectPtr<APlayerController> _BoundPlayerController;

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

	// 리스폰 등으로 폰이 바뀌면 투척물 종류 알림을 다시 연결
	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	void BindCharacter(APawn* Pawn);
	void UnBindCharacter();

	// 인벤토리 / 투척물 종류 변경 신호 수신 -> 다시 계산
	UFUNCTION()
	void UpdateGrenadeStatus();
};
