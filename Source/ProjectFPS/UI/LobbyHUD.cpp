// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyHUD.h"

void ALobbyHUD::BeginPlay()
{
	Super::BeginPlay();
	ShowScreen(_LobbyWidgetClass);	// 로비 메인 화면
	ApplyInputMode(true);			// 로비 = UI 모드 + 커서 -> 로비에서 버튼을 눌러야하기 때문에
}

