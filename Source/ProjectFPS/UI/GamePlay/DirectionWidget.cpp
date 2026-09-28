// Fill out your copyright notice in the Description page of Project Settings.
#include "UI/GamePlay/DirectionWidget.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"

void UDirectionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	APlayerController* OwningController = GetOwningPlayer();

	if(!OwningController)
	{
		return;
	}

	const float Yaw = OwningController->GetControlRotation().Yaw;

	// 자식 위젯 순서: N, NE, E, SE, S, SW, W, NW.
	// 0도는 +X(북쪽), 90도는 +Y(동쪽)이다.
	for (int32 Index = 0; Index < CompassPanel->GetChildrenCount(); ++Index)
	{
		UWidget* DirectionWidget = CompassPanel->GetChildAt(Index);

		const float DirectionAngle = Index * 45.0f;

		// 359도와 0도 사이에서도 가까운 방향으로 이어지게 한다.
		const float AngleDifference = FMath::FindDeltaAngleDegrees(Yaw, DirectionAngle);

		// 시선과 일치하는 방향은 중앙, 다른 방향은 좌우로 이동한다.
		DirectionWidget->SetRenderTranslation(FVector2D(AngleDifference * 6.0f, 0.0f));
	}
}
