// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DirectionWidget.generated.h"

/**
 *  화면 상단 12시방향에서 방향 나침반을 표시하는 위젯입니다.
 */

class UCanvasPanel;

UCLASS()
class PROJECTFPS_API UDirectionWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CompassPanel;
};
