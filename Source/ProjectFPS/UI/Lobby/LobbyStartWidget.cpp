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
#include "Misc/PackageName.h"

FFPSPlayableMap ULobbyStartWidget::GetQuickMatchMap() const
{
	if (!_SelectedQuickMap.Level.IsNull())
	{
		return _SelectedQuickMap;
	}

	const FString DefaultMapPath = GameLevel.ToSoftObjectPath().GetLongPackageName();
	if (const UFPSOnlineSessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (const FFPSPlayableMap* CatalogMap = Subsystem->FindPlayableMap(DefaultMapPath))
		{
			return *CatalogMap;
		}
	}

	FFPSPlayableMap DefaultMap;
	if (!GameLevel.IsNull() && FPackageName::IsValidLongPackageName(DefaultMapPath)
		&& FPackageName::DoesPackageExist(DefaultMapPath))
	{
		DefaultMap.Level = GameLevel;
		DefaultMap.DisplayName = FText::FromString(FPackageName::GetShortName(DefaultMapPath));
		DefaultMap.Mode = EFPSMatchMode::PVP;
	}
	return DefaultMap;
}

bool ULobbyStartWidget::SetQuickMatchMap(const FFPSPlayableMap& Map)
{
	const UFPSOnlineSessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!IsValid(Subsystem) || Subsystem->IsBusy() || Subsystem->IsAutoMatchInProgress()
		|| Subsystem->IsExitInProgress() || Subsystem->GetConnectionState() != EFPSOnlineConnectionState::None)
	{
		return false;
	}

	const FString MapPath = Map.GetMapPath();
	if (Map.Level.IsNull() || !FPackageName::IsValidLongPackageName(MapPath)
		|| !FPackageName::DoesPackageExist(MapPath))
	{
		return false;
	}

	if (const FFPSPlayableMap* CatalogMap = Subsystem->FindPlayableMap(MapPath, Map.Mode))
	{
		_SelectedQuickMap = *CatalogMap;
		return true;
	}

	// The original editor-configured level remains available when it is not in the catalog.
	if (!GameLevel.IsNull() && MapPath == GameLevel.ToSoftObjectPath().GetLongPackageName()
		&& Map.Mode == EFPSMatchMode::PVP)
	{
		FFPSPlayableMap DefaultMap;
		DefaultMap.Level = GameLevel;
		DefaultMap.DisplayName = FText::FromString(FPackageName::GetShortName(MapPath));
		_SelectedQuickMap = MoveTemp(DefaultMap);
		return true;
	}

	return false;
}

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
	if (IsValid(StartButton))
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnStartClicked);
	}

	if (UFPSOnlineSessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->_OnAutoMatchCompleted.AddUniqueDynamic(this, &ThisClass::HandleAutoMatchCompleted);
		Subsystem->_OnTravelFailed.AddUniqueDynamic(this, &ThisClass::HandleSessionTravelFailed);
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
	const FFPSPlayableMap SelectedMap = GetQuickMatchMap();
	// 레벨 미지정 방어.
	if (true == SelectedMap.Level.IsNull())
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

	const FString MapPath = SelectedMap.GetMapPath();
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
	Options._GameModeId = FPSMatchModeUtils::ToId(SelectedMap.Mode);

	/**
	* 참가 가능한 Session이 있으면 Guest로 들어가고, 없으면 직접 Host가 된다.
	* 후보 참가 실패 같은 중간 과정은 통지되지 않으므로
	* 결과는 HandleAutoMatchCompleted에서 한 번만 받는다.
	*/
	_QuickMatchRequested = true;
	if (false == SessionSubsystem->AutoJoinOrHost(Options, 100, true))
	{
		_QuickMatchRequested = false;
		SetStartButtonEnabled(true);
	}
}

void ULobbyStartWidget::HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage)
{
	if (!_QuickMatchRequested)
	{
		return;
	}

	_QuickMatchRequested = false;
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
	if (!_QuickMatchRequested)
	{
		return;
	}

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
