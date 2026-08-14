// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyStartWidget.h"
#include "Components/Button.h"
#include "Engine/World.h"

void ULobbyStartWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &ULobbyStartWidget::OnStartCliced);
	}
}

void ULobbyStartWidget::OnStartCliced()
{
	// 레벨 미지정 방어.
	if (GameLevel.IsNull())
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// 소프트 참조 -> 패키지 경로 문자열
	const FString MapPath = GameLevel.ToSoftObjectPath().GetLongPackageName();

	//게임 레벨로 이동 
	World->ServerTravel(MapPath);

}
