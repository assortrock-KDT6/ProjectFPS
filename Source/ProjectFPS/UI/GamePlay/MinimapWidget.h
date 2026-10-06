// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"


class UImage;
class APawn;

/**
 * 로컬 플레이어의 컴포넌트가 만든 화면을 띄운다.
 * 배경을 어디서 가져오는지 이 위젯이 알고 마커 등은 위에 얹는다.
 * 나중에 정적 지도 방식으로 바꾸면 이 위젯의 이미지 소스만 교체하면된다.
 */
UCLASS()
class PROJECTFPS_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

	// MapTexture 파라미터를 가진 UI 머티리얼을 브러시로 사용하는 이미지.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _MapImage;

	// 중앙 플레이어 마커(코드가 건드리지 않는다.) -> 자기자신이 미니맵에 뜨는 중앙점.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> _PlayerMarker;

	// 지금 연결된 폰. 리스폰으로 바뀌면 다시 연결함.
	TWeakObjectPtr<APawn> _BoundPawn;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 폰의 캡쳐를 머티리얼에 연결. Pawn이 null이면 배경만 숨긴다.
	bool SetupFromPawn(APawn* Pawn);
};
