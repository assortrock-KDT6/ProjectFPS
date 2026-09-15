// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GameHUD.h"
#include "UI/GamePlay/GameMainWidget.h"



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
	UE_LOG(LogTemp, Warning, TEXT("ToggleInventory 호출"));

	CloseOverlay(_MapWidget); //맵이 열려있으면 닫기 -> *나중에 묶던가 고민.
	
	const bool bOpen = ToggleOverlay(_InventoryWidgetClass, _InventoryWidget);
	ApplyInputMode(bOpen);

	// 인벤이 열리면 툴팁도 정리
	if (bOpen)
		HideItemInfo();

	// 맵이 열리면 툴팁도 정리
	if (bOpen)
		HideItemInfo();

	UE_LOG(LogTemp, Warning, TEXT("결과: bOpen=%d, Widget=%s"),
		bOpen, _InventoryWidget ? TEXT("생성됨") : TEXT("null"));
}

void AGameHUD::ToggleMap()
{
	CloseOverlay(_InventoryWidget); // 인벤토리 열려있으면 닫기 -> "" 동일

	ToggleOverlay(_MapWidgetClass, _MapWidget);
}

void AGameHUD::ShowItemInfo(FName TID)
{
	// 인벤,맵이 떠 있는 동안 월드 툴팁을 안띄움.
	if (nullptr != _InventoryWidget || nullptr != _MapWidget)
		return;

	UGameMainWidget* Main = Cast<UGameMainWidget>(_CurrentScreen);
	if (nullptr == Main)
		return;

	// MainWidget로 보냄
	Main->ShowItemInfo(TID);
}

void AGameHUD::HideItemInfo()
{
	UGameMainWidget* Main = Cast<UGameMainWidget>(_CurrentScreen);
	if (nullptr == Main)
		return;

	// MainWidget로 보냄
	Main->HideItemInfo();
}

void AGameHUD::ApplyInputMode(bool bUIMode)
{
	
	APlayerController* PC = GetOwningPlayerController();
	if (nullptr == PC)
		return;

	if (bUIMode)
	{
		// 커서를 쓰면서 게임 입력(닫기 키)도 유지
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

