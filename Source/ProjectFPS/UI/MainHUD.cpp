// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainHUD.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/ExitMenuWidget.h"
#include "Character/CharacterPlayer.h"
#include "Engine/GameInstance.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"

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

bool AMainHUD::ToggleOverlay(TSubclassOf<UUserWidget> OverlayClass, TObjectPtr<UUserWidget>& OverlayPtr, int32 ZOrder)
{
    
    if(OverlayPtr)  //이미 창이 존재함.
    {
        OverlayPtr->RemoveFromParent();
        OverlayPtr = nullptr;
        return false;   //닫혔다고 전달.
    }
    
    OverlayPtr = CreateWidget<UUserWidget>(GetOwningPlayerController(), OverlayClass);

    if (OverlayPtr)
    {
        OverlayPtr->AddToViewport(ZOrder);
    }
    return OverlayPtr != nullptr;   // 열렸다고 알림
}

void AMainHUD::ApplyInputMode(bool bUIMode)
{
    //PC : PlayerController

    APlayerController* PC = GetOwningPlayerController();  // 
    
    if (!PC)
    {
        return;
    }

    if (bUIMode || IsExitMenuOpen())
    {
        FInputModeUIOnly Mode;

        UUserWidget* Focus = IsExitMenuOpen() ? static_cast<UUserWidget*>(ExitMenuWidget.Get()) : _CurrentScreen.Get();

        if (Focus)
        {
            Mode.SetWidgetToFocus(Focus->TakeWidget());
        }

        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->SetInputMode(Mode);

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

void AMainHUD::ToggleExitMenu()
{
    if (!IsLocalHUD())
    {
        return;
    }

    if (IsExitMenuOpen()) 
    { 
        CloseExitMenu(); 
        return; 
    }

    APlayerController* PC = GetOwningPlayerController();

    ExitMenuWidget = CreateExitMenu();

    if (!ExitMenuWidget)
    {
        return;
    }

    if (auto* Character = Cast<ACharacterPlayer>(PC->GetPawn())) 
    {
        Character->StopAttacking();
    }

    PC->SetIgnoreMoveInput(true);

    PC->SetIgnoreLookInput(true);

    ExitMenuWidget->AddToViewport(2000);

    ApplyInputMode(true);

    ExitMenuWidget->SetUserFocus(PC);
}

void AMainHUD::CloseExitMenu()
{
    if (!ExitMenuWidget)
    {
        return;
    }
    auto* Sessions = GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>();

    if (Sessions && Sessions->IsExitInProgress())
    {
        return;
    }

    ExitMenuWidget->RemoveFromParent();
    ExitMenuWidget = nullptr;

    if (APlayerController* PC = GetOwningPlayerController())
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        RestoreInputAfterExitMenu();
    }
}

bool AMainHUD::IsExitMenuOpen() const
{
    return ExitMenuWidget != nullptr;
}

void AMainHUD::RestoreInputAfterExitMenu()
{
    ApplyInputMode(true);
}

void AMainHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ExitMenuWidget)
    {
        ExitMenuWidget->RemoveFromParent();
        ExitMenuWidget = nullptr;
        if (APlayerController* PC = GetOwningPlayerController())
        {
            PC->SetIgnoreMoveInput(false);
            PC->SetIgnoreLookInput(false);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AMainHUD::CloseOverlay(TObjectPtr<UUserWidget>& OverlayPtr)
{
    if (OverlayPtr)
    {
        OverlayPtr->RemoveFromParent();
        OverlayPtr = nullptr;
    }
}

