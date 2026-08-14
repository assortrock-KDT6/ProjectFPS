// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GameHUD.h"

void AGameHUD::SwitchTo(EMatchPhase Phase)
{
	// 서버 체크 하고 

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
	CloseOverlay(_MapWidget); //맵이 열려있으면 닫기 -> *나중에 묶던가 고민.
	
	ToggleOverlay(_InventoryWidgetClass, _InventoryWidget);

	//const bool bOpen = ToggleOverlay(_InventoryWidgetClass, _InventoryWidget);
	//ApplyInputMode(bOpen); //열림 -> UI 커서 On 닫흠 -> 게임 입력.
}

void AGameHUD::ToggleMap()
{
	CloseOverlay(_InventoryWidget); // 인베토리 열려있으면 닫기 -> "" 동일

	ToggleOverlay(_MapWidgetClass, _MapWidget);
}
