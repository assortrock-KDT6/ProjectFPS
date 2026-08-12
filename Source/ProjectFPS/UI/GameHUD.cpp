// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GameHUD.h"

void AGameHUD::SwitchTo(EMatchPhase Phase)
{
	TSubclassOf<UUserWidget> ClassToShow = nullptr;	// 출력 위젯을 담을 변수
	bool bUIMode = false;							// 입력 모드를 담을 변수

	// 보여질 화면 선택.
	switch (Phase)
	{
	case EMatchPhase::Waiting:
		ClassToShow = _WaitingWidgetClass;
		bUIMode = false;
		break;
	case EMatchPhase::GamePlay:
		ClassToShow = _GameplayWidgetClass;
		bUIMode = false;
		break;
	}
	ShowScreen(ClassToShow);
	ApplyInputMode(bUIMode);

}

void AGameHUD::ToggleInventory()
{
	ToggleOverlay(_InventoryWidgetClass, _InventoryWidget);
}
