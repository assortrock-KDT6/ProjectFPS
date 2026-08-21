// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyStartWidget.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Common/GameDatas.h"

UFPSOnlineSessionSubsystem* ULobbyStartWidget::GetSessionSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (nullptr == GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UFPSOnlineSessionSubsystem>();
}

void ULobbyStartWidget::SetStartButtonEnabled(bool IsEnabled)
{
	if (true == IsValid(StartButton))
	{
		StartButton->SetIsEnabled(IsEnabled);
	}
}

void ULobbyStartWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (true == IsValid(StartButton))
	{
		StartButton->OnClicked.AddDynamic(this, &ThisClass::OnStartClicked);
	}

	UFPSOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (nullptr != SessionSubsystem)
	{
		SessionSubsystem->_OnAutoMatchCompleted.AddUniqueDynamic(this, &ThisClass::HandleAutoMatchCompleted);
		SessionSubsystem->_OnTravelFailed.AddUniqueDynamic(this, &ThisClass::HandleSessionTravelFailed);
	}
}

void ULobbyStartWidget::NativeDestruct()
{
	UFPSOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (nullptr != SessionSubsystem)
	{
		SessionSubsystem->_OnAutoMatchCompleted.RemoveDynamic(this, &ThisClass::HandleAutoMatchCompleted);
		SessionSubsystem->_OnTravelFailed.RemoveDynamic(this, &ThisClass::HandleSessionTravelFailed);
	}

	if (true == IsValid(StartButton))
	{
		StartButton->OnClicked.RemoveDynamic(this, &ThisClass::OnStartClicked);
	}

	Super::NativeDestruct();
}

void ULobbyStartWidget::OnStartClicked()
{
	// 레벨 미지정 방어.
	if (true == GameLevel.IsNull())
	{
		LastSessionError = TEXT("Game level is not configured.");
		return;
	}

	UFPSOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (nullptr == SessionSubsystem)
	{
		LastSessionError = TEXT("Online Session subsystem is unavailable.");
		return;
	}

	const FString MapPath = GameLevel.ToSoftObjectPath().GetLongPackageName();
	if (true == MapPath.IsEmpty())
	{
		LastSessionError = TEXT("Game Level Package path is invalid.");
		return;
	}

	LastSessionError.Reset();
	SetStartButtonEnabled(false);

	FFPSSessionCreateOptions Options;
	Options._MaxPlayers = _PublicConnections;
	Options._MapId		= MapPath;

	/**
	* 참가 가능한 Session이 있으면 Guest로 들어가고, 없으면 직접 Host가 된다.
	* 후보 참가 실패 같은 중간 과정은 통지되지 않으므로
	* 결과는 HandleAutoMatchCompleted에서 한 번만 받는다.
	*/
	if (false == SessionSubsystem->AutoJoinOrHost(Options))
	{
		SetStartButtonEnabled(true);
	}
}

void ULobbyStartWidget::HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage)
{
	LastMatchIsHost = IsHost;

	if (true == WasSuccessful)
	{
		LastSessionError.Reset();
		return;
	}

	LastSessionError = ErrorMessage;
	SetStartButtonEnabled(true);
}

void ULobbyStartWidget::HandleSessionTravelFailed(const FString& ErrorMessage)
{
	LastSessionError = ErrorMessage;

	/**
	* 자동 매치가 아직 다음 후보나 Host 전환을 시도하는 중이면 최종 실패가 아니다.
	* 이때 버튼을 되살리면 진행 중에 중복 요청이 들어올 수 있다.
	*/
	const UFPSOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (nullptr != SessionSubsystem && true == SessionSubsystem->IsAutoMatchInProgress())
	{
		return;
	}

	SetStartButtonEnabled(true);
}
