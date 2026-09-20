// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyStartWidget.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Common/GameDatas.h"

void ULobbyStartWidget::ClearSessionError()
{
	if (SessionErrorOverlay.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(SessionErrorOverlay.ToSharedRef());
	}
	SessionErrorOverlay.Reset();
	LastSessionError.Reset();
}

void ULobbyStartWidget::ShowSessionError(const FString& ErrorMessage)
{
	ClearSessionError();
	LastSessionError = ErrorMessage;
	if (UFPSOnlineSessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->SetLastSessionError(ErrorMessage);
	}
	UE_LOG(LogTemp, Warning, TEXT("Lobby session error: %s"), *ErrorMessage);
	if (GetWorld() && GetWorld()->GetGameViewport())
	{
		SessionErrorOverlay = SNew(SBox)
			.Visibility(EVisibility::HitTestInvisible)
			.HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24.f, 24.f, 24.f, 80.f))
			[
				SNew(SBorder).Padding(16.f)
				[
					SNew(STextBlock).Text(FText::FromString(ErrorMessage))
					.ColorAndOpacity(FLinearColor::White).WrapTextAt(800.f)
				]
			];
		GetWorld()->GetGameViewport()->AddViewportWidgetContent(SessionErrorOverlay.ToSharedRef(), 100);
	}
}

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
	ClearSessionError();
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

void ULobbyStartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UFPSOnlineSessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		SetStartButtonEnabled(!Subsystem->IsBusy() && !Subsystem->IsAutoMatchInProgress());
		const FString Error = Subsystem->GetLastSessionError();
		if (!Error.IsEmpty())
		{
			ShowSessionError(Error);
		}
	}
}

void ULobbyStartWidget::OnStartClicked()
{
	// 레벨 미지정 방어.
	if (true == GameLevel.IsNull())
	{
		ShowSessionError(TEXT("Game level is not configured."));
		return;
	}

	UFPSOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (nullptr == SessionSubsystem)
	{
		ShowSessionError(TEXT("Online Session subsystem is unavailable."));
		return;
	}

	const FString MapPath = GameLevel.ToSoftObjectPath().GetLongPackageName();
	if (true == MapPath.IsEmpty())
	{
		ShowSessionError(TEXT("Game Level Package path is invalid."));
		return;
	}

	ClearSessionError();
	SessionSubsystem->SetLastSessionError(FString());
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
		ClearSessionError();
		return;
	}

	ShowSessionError(ErrorMessage);
	SetStartButtonEnabled(true);
}

void ULobbyStartWidget::HandleSessionTravelFailed(const FString& ErrorMessage)
{
	ShowSessionError(ErrorMessage);

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
