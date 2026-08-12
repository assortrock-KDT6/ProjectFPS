// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainHUD.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

bool AMainHUD::IsLocalHUD() const
{
    APlayerController* PC = GetOwningPlayerController();
    return PC && PC->IsLocalController();
}


UUserWidget* AMainHUD::ShowScreen(TSubclassOf<UUserWidget> ScreenClass)
{
    // 체크
    if (!IsLocalHUD() || !ScreenClass)
    {
        return nullptr;
    }

    // 기존 화면 제거
    RemoveCurrentScreen();

    //GetOwningPlayerController() : HUD 주인플레이어 컨트롤러 반환 

    // 새로운 위젯 화면 띄우기
    _CurrentScreen = CreateWidget<UUserWidget>(GetOwningPlayerController(), ScreenClass);
    if (_CurrentScreen)
    {
        _CurrentScreen->AddToViewport();
    }
    return _CurrentScreen;
}

void AMainHUD::RemoveCurrentScreen()
{
    if (_CurrentScreen)
    {
        _CurrentScreen->RemoveFromParent();  // 화면(viewport)에서 제거
        _CurrentScreen = nullptr;            // 현재 화면 없을 -> 비움처리
    }
}

void AMainHUD::ToggleOverlay(TSubclassOf<UUserWidget> OverlayClass, TObjectPtr<UUserWidget>& OverlayPtr, int32 ZOrder)
{
    if(OverlayPtr)
    {
        OverlayPtr->RemoveFromParent();
        OverlayPtr = nullptr;
    }
    else
    {
        OverlayPtr = CreateWidget<UUserWidget>(GetOwningPlayerController(), OverlayClass);
        if (OverlayPtr)
        {
            OverlayPtr->AddToViewport(ZOrder);
        }
    }
}

void AMainHUD::ApplyInputMode(bool bUIMode)
{
    //PC : PlayerController

    APlayerController* PC = GetOwningPlayerController();  // 
    
    if (!PC)
    {
        return;
    }

    if (bUIMode)
    {
        PC->SetInputMode(FInputModeUIOnly());
        PC->SetShowMouseCursor(true);
    }
    else
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->SetShowMouseCursor(false);
    }
}

void AMainHUD::ToggleSettings()
{
    ToggleOverlay(SettingWidgetClass, SettingWidget);
}

