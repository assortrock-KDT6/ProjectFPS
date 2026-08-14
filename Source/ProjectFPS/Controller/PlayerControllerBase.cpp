// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/PlayerControllerBase.h"

APlayerControllerBase::APlayerControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// 로비의 UI 입력 모드 잔재 방빚 -> 게임 진입 시 게임 입력 모드로 명시
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void APlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void APlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();
}