// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyMainWidget.h"
#include "UI/Lobby/LobbyMenuWidget.h"
#include "Components/WidgetSwitcher.h"
#include "UI/MainHUD.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

void ULobbyMainWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	// 메뉴 위젯의 "탭 선택 신호" 를 구독 -> 버튼이 눌리면 HandleTabSelected 실행
	if (LobbyMenu)
	{
		LobbyMenu->OnTabSelected.AddDynamic(this, &ULobbyMainWidget::HandleTabSelected);
	}
}

FReply ULobbyMainWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	// Bubble phase lets the existing session picker handle its own ESC first.
	if (Event.GetKey() == EKeys::Escape)
	{
		if (!Event.IsRepeat())
		{
			if (auto* HUD = GetOwningPlayer() ? Cast<AMainHUD>(GetOwningPlayer()->GetHUD()) : nullptr)
			{
				HUD->ToggleExitMenu();
			}

		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

// 받은 인덱스로 스위처 패널 전환 
void ULobbyMainWidget::HandleTabSelected(int32 Index)
{
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(Index);
	}
}
