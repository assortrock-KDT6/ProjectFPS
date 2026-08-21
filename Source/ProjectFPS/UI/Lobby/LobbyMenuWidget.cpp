// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyMenuWidget.h"
#include "Components/Button.h"


void ULobbyMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 초기화 시 1회 각 버튼에 핸들러 연결
	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &ULobbyMenuWidget::OnPlayClicked);
	}
	if (ShopButton)
	{
		ShopButton->OnClicked.AddDynamic(this, &ULobbyMenuWidget::OnShopClicked);
	}
	if (SettingButton)
	{
		SettingButton->OnClicked.AddDynamic(this, &ULobbyMenuWidget::OnSettingClicked);
	}
}

// 버튼별 자기 탭 인덱스를 발송함 (스위처 패널 순서와 인덱스가 일치해야함.)

void ULobbyMenuWidget::OnPlayClicked()
{
	OnTabSelected.Broadcast(0);
}

void ULobbyMenuWidget::OnShopClicked()
{
	OnTabSelected.Broadcast(1);
}

void ULobbyMenuWidget::OnSettingClicked()
{
	OnTabSelected.Broadcast(2);
}
