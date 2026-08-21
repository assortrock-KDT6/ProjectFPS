// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyMainWidget.h"
#include "UI/Lobby/LobbyMenuWidget.h"
#include "Components/WidgetSwitcher.h"

void ULobbyMainWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 메뉴 위젯의 "탭 선택 신호" 를 구독 -> 버튼이 눌리면 HandleTabSelected 실행
	if (LobbyMenu)
		LobbyMenu->OnTabSelected.AddDynamic(this, &ULobbyMainWidget::HandleTabSelected);

}

// 받은 인덱스로 스위처 패널 전환 
void ULobbyMainWidget::HandleTabSelected(int32 Index)
{
	if (ContentSwitcher)
		ContentSwitcher->SetActiveWidgetIndex(Index);

}
