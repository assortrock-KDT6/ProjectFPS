#include "UI/Lobby/LobbyPlayPanelWidget.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "UI/Lobby/LobbyStartWidget.h"
#include "UI/Session/SessionMenuWidget.h"

void ULobbyPlayPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	_SessionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenSessionMenu);
	_SessionMenu->_OnCloseRequested.AddUniqueDynamic(this, &ThisClass::CloseSessionMenu);
	_SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>() : nullptr;

	if (IsValid(_SessionSubsystem))
	{
		_SessionSubsystem->_OnOperationStateChanged.AddUniqueDynamic(this, &ThisClass::HandleOperationChanged);
		_SessionSubsystem->_OnConnectionStateChanged.AddUniqueDynamic(this, &ThisClass::HandleConnectionChanged);
		_SessionSubsystem->_OnTravelStateChanged.AddUniqueDynamic(this, &ThisClass::HandleTravelChanged);
		_SessionSubsystem->_OnAutoMatchCompleted.AddUniqueDynamic(this, &ThisClass::HandleAutoMatchCompleted);
		// 생성/참가 후 로비 이동 시에는 연결된 방의 화면을 이어서 표시한다.
		_SessionMenuOpen = _SessionSubsystem->GetConnectionState() != EFPSOnlineConnectionState::None;
	}

	UpdateLauncher();
}

void ULobbyPlayPanelWidget::NativeDestruct()
{
	_SessionButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenSessionMenu);
	_SessionMenu->_OnCloseRequested.RemoveDynamic(this, &ThisClass::CloseSessionMenu);

	if (IsValid(_SessionSubsystem))
	{
		_SessionSubsystem->_OnOperationStateChanged.RemoveDynamic(this, &ThisClass::HandleOperationChanged);
		_SessionSubsystem->_OnConnectionStateChanged.RemoveDynamic(this, &ThisClass::HandleConnectionChanged);
		_SessionSubsystem->_OnTravelStateChanged.RemoveDynamic(this, &ThisClass::HandleTravelChanged);
		_SessionSubsystem->_OnAutoMatchCompleted.RemoveDynamic(this, &ThisClass::HandleAutoMatchCompleted);
	}

	Super::NativeDestruct();
}

void ULobbyPlayPanelWidget::OpenSessionMenu()
{
	_SessionMenuOpen = true;
	UpdateLauncher();

	if (GetOwningPlayer())
	{
		_SessionMenu->SetUserFocus(GetOwningPlayer());
	}
}

void ULobbyPlayPanelWidget::CloseSessionMenu()
{
	_SessionMenuOpen = false;
	UpdateLauncher();

	if (GetOwningPlayer())
	{
		_SessionButton->SetUserFocus(GetOwningPlayer());
	}
}

void ULobbyPlayPanelWidget::UpdateLauncher()
{
	// 닫기는 UI만 숨긴다. 접속 중인 세션을 종료하지 않는다.
	_SessionMenu->SetVisibility(_SessionMenuOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	_LauncherPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const bool CanQuickMatch = !_SessionMenuOpen && IsValid(_SessionSubsystem) && !_SessionSubsystem->IsBusy()
		&& !_SessionSubsystem->IsAutoMatchInProgress()
		&& _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::None;
	_QuickMatchWidget->SetIsEnabled(CanQuickMatch);
}

void ULobbyPlayPanelWidget::HandleOperationChanged(EFPSOnlineOperationState State)
{
	UpdateLauncher();
}

void ULobbyPlayPanelWidget::HandleConnectionChanged(EFPSOnlineConnectionState State)
{
	UpdateLauncher();
}

void ULobbyPlayPanelWidget::HandleTravelChanged(EFPSOnlineTravelState State)
{
	UpdateLauncher();
}

void ULobbyPlayPanelWidget::HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage)
{
	UpdateLauncher();
}
