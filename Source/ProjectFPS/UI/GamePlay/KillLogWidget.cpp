// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/KillLogWidget.h"
#include "UI/GamePlay/KillLogEntryWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "GameMode/FPSGameState.h"
#include "Components/VerticalBox.h"


void UKillLogWidget::NativeConstruct()
{
	if (nullptr == GetWorld())
	{
		return;
	}

	Super::NativeConstruct();

	if (AFPSGameState* GameState = GetWorld()->GetGameState<AFPSGameState>())
	{
		GameState->_OnKillLogged.AddUniqueDynamic(this, &UKillLogWidget::AddKillLog);
		_GameState = GameState;
	}
}

void UKillLogWidget::NativeDestruct()
{
	if (nullptr != _GameState)
	{
		_GameState->_OnKillLogged.RemoveDynamic(this, &UKillLogWidget::AddKillLog);
	}

	Super::NativeDestruct();
}

void UKillLogWidget::AddKillLog(const FPlayerKillLogResult& Result)
{
	if (nullptr == _KillLogBox || nullptr == _KillLogEntryClass || !Result.IsValid())
	{
		return;
	}

	UKillLogEntryWidget* Entry = CreateWidget<UKillLogEntryWidget>(GetOwningPlayer(), 
		_KillLogEntryClass);

	if (nullptr != Entry && Entry->SetEntryInformation(Result))
	{
		_KillLogBox->AddChildToVerticalBox(Entry);
	}
}
