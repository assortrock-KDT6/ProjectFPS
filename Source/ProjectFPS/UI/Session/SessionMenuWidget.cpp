#include "UI/Session/SessionMenuWidget.h"
#include "UI/Session/SessionEntryWidget.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "InputCoreTypes.h"

void USessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	_SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>() : nullptr;
	_CreateButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CreateSession);
	_CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseMenu);
	_FindTabButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowFindPanel);
	_CreateTabButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowCreatePanel);
	SetIsFocusable(true);
	_RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RefreshSessions);
	_StartMatchButton->OnClicked.AddUniqueDynamic(this, &ThisClass::StartMatch);
	_LeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RequestLeave);
	_ConfirmLeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ConfirmLeave);
	_CancelLeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CancelLeave);
	_ExitConfirmation->SetVisibility(ESlateVisibility::Collapsed);
	if (!_InputsInitialized)
	{
		_RoomNameInput->SetText(FText::FromString(TEXT("데스매치 로비")));
		_MaxPlayersInput->SetMinValue(2.f);
		_MaxPlayersInput->SetMaxValue(16.f);
		_MaxPlayersInput->SetDelta(1.f);
		_MaxPlayersInput->SetValue(FMath::Clamp(_DefaultMaxPlayers, 2, 16));
		_AllowJoinInProgressCheck->SetIsChecked(true);
		_InputsInitialized = true;
	}
	// 제거 후 같은 위젯을 다시 열어도 이벤트를 재등록한다.
	BindSessionEvents();
	ClearSearchResults();
	if (IsValid(_SessionSubsystem))
	{
		SetNotice(_SessionSubsystem->GetLastSessionError(), !_SessionSubsystem->GetLastSessionError().IsEmpty());
	}
	else
	{
		SetNotice(TEXT("온라인 세션 서브시스템을 사용할 수 없습니다."), true);
	}
	UpdateDisplay();
	GetWorld()->GetTimerManager().SetTimer(_RefreshTimer, this, &ThisClass::UpdateDisplay, 0.25f, true);
}

void USessionMenuWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(_RefreshTimer);
	}
	UnbindSessionEvents();
	_CreateButton->OnClicked.RemoveDynamic(this, &ThisClass::CreateSession);
	_CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseMenu);
	_FindTabButton->OnClicked.RemoveDynamic(this, &ThisClass::ShowFindPanel);
	_CreateTabButton->OnClicked.RemoveDynamic(this, &ThisClass::ShowCreatePanel);
	_RefreshButton->OnClicked.RemoveDynamic(this, &ThisClass::RefreshSessions);
	_StartMatchButton->OnClicked.RemoveDynamic(this, &ThisClass::StartMatch);
	_LeaveButton->OnClicked.RemoveDynamic(this, &ThisClass::RequestLeave);
	_ConfirmLeaveButton->OnClicked.RemoveDynamic(this, &ThisClass::ConfirmLeave);
	_CancelLeaveButton->OnClicked.RemoveDynamic(this, &ThisClass::CancelLeave);
	ClearSearchResults();
	Super::NativeDestruct();
}

void USessionMenuWidget::CloseMenu()
{
	if (IsBusy())
	{
		return;
	}

	if (_ExitConfirmation->IsVisible())
	{
		CancelLeave();
		return;
	}

	_OnCloseRequested.Broadcast();
}

void USessionMenuWidget::ShowFindPanel()
{
	if (!CanBrowse())
	{
		return;
	}

	_CreatingRoom = false;
	UpdateDisplay();
}

void USessionMenuWidget::ShowCreatePanel()
{
	if (!CanBrowse())
	{
		return;
	}

	_CreatingRoom = true;
	UpdateDisplay();
}

FReply USessionMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		CloseMenu();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

void USessionMenuWidget::BindSessionEvents()
{
	if (!IsValid(_SessionSubsystem))
	{
		return;
	}
	_SessionSubsystem->_OnFindSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleFindCompleted);
	_SessionSubsystem->_OnCreateSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnJoinSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnLobbyReady.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnMatchStarted.AddUniqueDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnLeaveSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleExitCompleted);
	_SessionSubsystem->_OnDestroySessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleExitCompleted);
	_SessionSubsystem->_OnAutoMatchCompleted.AddUniqueDynamic(this, &ThisClass::HandleAutoMatchCompleted);
	_SessionSubsystem->_OnTravelFailed.AddUniqueDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnConnectionLost.AddUniqueDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnOperationStateChanged.AddUniqueDynamic(this, &ThisClass::HandleOperationChanged);
	_SessionSubsystem->_OnConnectionStateChanged.AddUniqueDynamic(this, &ThisClass::HandleConnectionChanged);
	_SessionSubsystem->_OnTravelStateChanged.AddUniqueDynamic(this, &ThisClass::HandleTravelChanged);
}

void USessionMenuWidget::UnbindSessionEvents()
{
	if (!IsValid(_SessionSubsystem))
	{
		return;
	}
	_SessionSubsystem->_OnFindSessionCompleted.RemoveDynamic(this, &ThisClass::HandleFindCompleted);
	_SessionSubsystem->_OnCreateSessionCompleted.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnJoinSessionCompleted.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnLobbyReady.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnMatchStarted.RemoveDynamic(this, &ThisClass::HandleRequestCompleted);
	_SessionSubsystem->_OnLeaveSessionCompleted.RemoveDynamic(this, &ThisClass::HandleExitCompleted);
	_SessionSubsystem->_OnDestroySessionCompleted.RemoveDynamic(this, &ThisClass::HandleExitCompleted);
	_SessionSubsystem->_OnAutoMatchCompleted.RemoveDynamic(this, &ThisClass::HandleAutoMatchCompleted);
	_SessionSubsystem->_OnTravelFailed.RemoveDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnConnectionLost.RemoveDynamic(this, &ThisClass::HandleSessionError);
	_SessionSubsystem->_OnOperationStateChanged.RemoveDynamic(this, &ThisClass::HandleOperationChanged);
	_SessionSubsystem->_OnConnectionStateChanged.RemoveDynamic(this, &ThisClass::HandleConnectionChanged);
	_SessionSubsystem->_OnTravelStateChanged.RemoveDynamic(this, &ThisClass::HandleTravelChanged);
}

bool USessionMenuWidget::IsBusy() const
{
	return !IsValid(_SessionSubsystem) || _ReturningToMenu || _ExitRequested
		|| _SessionSubsystem->IsBusy() || _SessionSubsystem->IsAutoMatchInProgress();
}

bool USessionMenuWidget::CanBrowse() const
{
	return !IsBusy() && _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::None;
}

void USessionMenuWidget::RefreshSessions()
{
	if (!CanBrowse())
	{
		return;
	}
	ClearNotice();
	ClearSearchResults();
	// 반환값은 요청 접수 여부다. 결과는 HandleFindCompleted에서만 표시한다.
	_SessionSubsystem->FindSessions(FMath::Max(1, _MaxSearchResults));
	UpdateDisplay();
}

bool USessionMenuWidget::BuildCreateOptions(FFPSSessionCreateOptions& Options)
{
	Options._DisplayName = _RoomNameInput->GetText().ToString().TrimStartAndEnd();
	if (Options._DisplayName.IsEmpty() || Options._DisplayName.Len() > 48)
	{
		SetNotice(TEXT("방 이름을 1~48자로 입력해 주세요."), true);
		return false;
	}
	if (_LobbyLevel.IsNull() || _GameLevel.IsNull())
	{
		SetNotice(TEXT("위젯 기본값에 Lobby Level과 Game Level을 지정해 주세요."), true);
		return false;
	}
	Options._MaxPlayers = FMath::Clamp(FMath::RoundToInt(_MaxPlayersInput->GetValue()), 2, 16);
	Options._AllowJoinProgress = _AllowJoinInProgressCheck->IsChecked();
	Options._MapId = _LobbyLevel.ToSoftObjectPath().GetLongPackageName();
	Options._GameModeId = TEXT("Deathmatch");
	return true;
}

void USessionMenuWidget::CreateSession()
{
	if (!CanBrowse())
	{
		return;
	}
	ClearNotice();
	FFPSSessionCreateOptions Options;
	if (!BuildCreateOptions(Options))
	{
		return;
	}
	ClearSearchResults();
	_SessionSubsystem->CreateSession(Options);
	UpdateDisplay();
}

void USessionMenuWidget::QuickJoin()
{
	if (!CanBrowse())
	{
		return;
	}
	ClearNotice();
	FFPSSessionCreateOptions Options;
	if (!BuildCreateOptions(Options))
	{
		return;
	}
	ClearSearchResults();
	_SessionSubsystem->AutoJoinOrHost(Options, FMath::Max(1, _MaxSearchResults));
	UpdateDisplay();
}

void USessionMenuWidget::JoinSession(int32 ResultIndex)
{
	if (!CanBrowse())
	{
		return;
	}
	bool JoinableResult = false;
	for (const FFPSOnlineSessionInfo& Session : _SearchResults)
	{
		if (Session._ResultIndex == ResultIndex && ResultIndex != INDEX_NONE
			&& Session._MaxPlayers > 0 && Session._CurrentPlayers < Session._MaxPlayers)
		{
			JoinableResult = true;
			break;
		}
	}
	if (!JoinableResult)
	{
		SetNotice(TEXT("참가할 수 없는 검색 결과입니다. 목록을 다시 검색해 주세요."), true);
		return;
	}
	ClearNotice();
	_SessionSubsystem->JoinSessionByIndex(ResultIndex);
	UpdateDisplay();
}

void USessionMenuWidget::StartMatch()
{
	if (IsBusy() || _ExitConfirmation->IsVisible()
		|| _SessionSubsystem->GetConnectionState() != EFPSOnlineConnectionState::Hosting)
	{
		return;
	}
	ClearNotice();
	if (_GameLevel.IsNull())
	{
		SetNotice(TEXT("Game Level이 지정되지 않았습니다."), true);
		return;
	}
	_SessionSubsystem->StartMatch(_GameLevel.ToSoftObjectPath().GetLongPackageName());
	UpdateDisplay();
}

void USessionMenuWidget::RequestLeave()
{
	if (IsBusy() || _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::None)
	{
		return;
	}
	const bool IsHost = _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::Hosting;
	_ExitConfirmationText->SetText(FText::FromString(IsHost
		? TEXT("방을 종료하면 모든 참가자의 연결이 끊어집니다. 종료할까요?")
		: TEXT("현재 세션에서 나가 로비 메뉴로 돌아갈까요?")));
	_ExitConfirmation->SetVisibility(ESlateVisibility::Visible);
	UpdateDisplay();
}

void USessionMenuWidget::ConfirmLeave()
{
	if (IsBusy() || !_ExitConfirmation->IsVisible()
		|| _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::None)
	{
		return;
	}
	if (_LobbyLevel.IsNull())
	{
		SetNotice(TEXT("돌아갈 Lobby Level이 지정되지 않았습니다."), true);
		return;
	}
	ClearNotice();
	_ExitRequested = true;
	_ExitConfirmation->SetVisibility(ESlateVisibility::Collapsed);
	// 로컬 세션 정리가 성공한 뒤에만 연결을 끊고 로비 메뉴를 연다.
	if (_SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::Hosting)
	{
		_SessionSubsystem->DestroySession();
	}
	else
	{
		_SessionSubsystem->LeaveSession();
	}
	UpdateDisplay();
}

void USessionMenuWidget::CancelLeave()
{
	_ExitConfirmation->SetVisibility(ESlateVisibility::Collapsed);
	UpdateDisplay();
}

void USessionMenuWidget::HandleExitCompleted(bool WasSuccessful, const FString& ErrorMessage)
{
	if (!_ExitRequested)
	{
		return;
	}
	_ExitRequested = false;
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
		UpdateDisplay();
		return;
	}
	ReturnToMenu();
}

void USessionMenuWidget::ReturnToMenu()
{
	_ReturningToMenu = true;
	ClearNotice();
	UGameplayStatics::OpenLevel(this, FName(*_LobbyLevel.ToSoftObjectPath().GetLongPackageName()), true);
}

void USessionMenuWidget::ClearSearchResults()
{
	for (USessionEntryWidget* Row : _SessionRows)
	{
		if (IsValid(Row))
		{
			Row->_OnJoinRequested.RemoveDynamic(this, &ThisClass::JoinSession);
		}
	}
	_SessionRows.Reset();
	_SearchResults.Reset();
	_SessionList->ClearChildren();
	_ResultCountText->SetText(FText::FromString(TEXT("0 ROOMS")));
	_EmptyListText->SetText(FText::FromString(TEXT("새로고침으로 참가 가능한 방을 찾아보세요.")));
	_EmptyListText->SetVisibility(ESlateVisibility::Visible);
}

void USessionMenuWidget::HandleFindCompleted(bool WasSuccessful, const TArray<FFPSOnlineSessionInfo>& Sessions, const FString& ErrorMessage)
{
	// 자동 매칭의 중간 검색 결과/실패는 최종 결과가 아니다.
	if (_SessionSubsystem->IsAutoMatchInProgress())
	{
		return;
	}
	ClearSearchResults();
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
		UpdateDisplay();
		return;
	}
	if (!_SessionEntryClass)
	{
		SetNotice(TEXT("Session Entry Class가 지정되지 않았습니다."), true);
		return;
	}
	_SearchResults = Sessions;
	for (const FFPSOnlineSessionInfo& Session : Sessions)
	{
		USessionEntryWidget* Row = CreateWidget<USessionEntryWidget>(GetOwningPlayer(), _SessionEntryClass);
		if (!IsValid(Row))
		{
			continue;
		}
		Row->DisplaySession(Session);
		Row->_OnJoinRequested.AddUniqueDynamic(this, &ThisClass::JoinSession);
		_SessionList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		_SessionRows.Add(Row);
	}
	_ResultCountText->SetText(FText::FromString(FString::Printf(TEXT("%d ROOMS"), _SessionRows.Num())));
	_EmptyListText->SetText(FText::FromString(TEXT("검색된 방이 없습니다. 방을 만들거나 다시 검색해 주세요.")));
	_EmptyListText->SetVisibility(_SessionRows.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetNotice(FString::Printf(TEXT("검색 완료 · %d개의 방을 찾았습니다."), _SessionRows.Num()));
	UpdateDisplay();
}

void USessionMenuWidget::HandleRequestCompleted(bool WasSuccessful, const FString& ErrorMessage)
{
	if (_SessionSubsystem->IsAutoMatchInProgress())
	{
		return;
	}
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage)
{
	if (!WasSuccessful)
	{
		SetNotice(ErrorMessage, true);
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleSessionError(const FString& ErrorMessage)
{
	if (_SessionSubsystem->IsAutoMatchInProgress())
	{
		return;
	}
	SetNotice(ErrorMessage, true);
	UpdateDisplay();
}

void USessionMenuWidget::HandleOperationChanged(EFPSOnlineOperationState State)
{
	if (State == EFPSOnlineOperationState::Finding)
	{
		ClearSearchResults();
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleConnectionChanged(EFPSOnlineConnectionState State)
{
	if (State != EFPSOnlineConnectionState::None)
	{
		ClearSearchResults();
	}
	UpdateDisplay();
}

void USessionMenuWidget::HandleTravelChanged(EFPSOnlineTravelState State)
{
	UpdateDisplay();
}

void USessionMenuWidget::SetNotice(const FString& Message, bool IsError)
{
	_NoticeText->SetText(FText::FromString(Message));
	_NoticeText->SetColorAndOpacity(IsError ? FLinearColor(1.f, 0.3f, 0.25f) : FLinearColor(0.72f, 0.75f, 0.78f));
	_NoticeText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (IsError && IsValid(_SessionSubsystem))
	{
		_SessionSubsystem->SetLastSessionError(Message);
	}
}

void USessionMenuWidget::ClearNotice()
{
	SetNotice(FString());
	if (IsValid(_SessionSubsystem))
	{
		_SessionSubsystem->SetLastSessionError(FString());
	}
}

void USessionMenuWidget::UpdateDisplay()
{
	const bool Ready = !IsBusy();
	const EFPSOnlineConnectionState Connection = IsValid(_SessionSubsystem)
		? _SessionSubsystem->GetConnectionState() : EFPSOnlineConnectionState::None;
	const bool Connected = Connection != EFPSOnlineConnectionState::None;
	const bool IsHost = Connection == EFPSOnlineConnectionState::Hosting;
	const bool Confirming = _ExitConfirmation->IsVisible();
	_BrowsePanel->SetVisibility(Connected ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	_ConnectedPanel->SetVisibility(Connected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	_CreateButton->SetIsEnabled(CanBrowse());
	_CloseButton->SetIsEnabled(Ready);
	_FindTabButton->SetIsEnabled(CanBrowse());
	_CreateTabButton->SetIsEnabled(CanBrowse());
	_FindPanel->SetVisibility(_CreatingRoom ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	_CreatePanel->SetVisibility(_CreatingRoom ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	_FindTabIndicator->SetVisibility(_CreatingRoom ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	_CreateTabIndicator->SetVisibility(_CreatingRoom ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	_RefreshButton->SetIsEnabled(CanBrowse());
	_RoomNameInput->SetIsEnabled(CanBrowse());
	_MaxPlayersInput->SetIsEnabled(CanBrowse());
	_AllowJoinInProgressCheck->SetIsEnabled(CanBrowse());
	_StartMatchButton->SetVisibility(IsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	_StartMatchButton->SetIsEnabled(Ready && IsHost && !Confirming);
	_LeaveButton->SetIsEnabled(Ready && Connected && !Confirming);
	_LeaveButtonText->SetText(FText::FromString(IsHost ? TEXT("방 종료") : TEXT("나가기 / 연결 정리")));
	_ConfirmLeaveButton->SetIsEnabled(Ready && Connected);
	_CancelLeaveButton->SetIsEnabled(Ready);
	for (USessionEntryWidget* Row : _SessionRows)
	{
		Row->SetJoinAllowed(CanBrowse());
	}
	if (!Connected)
	{
		_ExitConfirmation->SetVisibility(ESlateVisibility::Collapsed);
	}

	FString Status = TEXT("방을 찾거나 새로운 데스매치 로비를 만드세요.");
	if (IsValid(_SessionSubsystem))
	{
		const FName Service = _SessionSubsystem->GetOnlineServiceName();
		_ServiceText->SetText(FText::FromString(Service == FName(TEXT("NULL")) ? TEXT("LAN / NULL") : Service.ToString()));
		if (IsHost)
		{
			Status = TEXT("HOST · 참가자를 기다리는 중");
		}
		else if (Connection == EFPSOnlineConnectionState::Joined)
		{
			Status = TEXT("GUEST · 호스트의 게임 시작을 기다리는 중");
		}
		else if (Connection == EFPSOnlineConnectionState::CleanupFailed)
		{
			Status = TEXT("연결 정리가 필요합니다. 나가기 버튼으로 다시 시도해 주세요.");
		}
		switch (_SessionSubsystem->GetOperationState())
		{
		case EFPSOnlineOperationState::Finding:
			Status = TEXT("참가 가능한 방을 검색하는 중...");
			break;
		case EFPSOnlineOperationState::Creating:
			Status = TEXT("새로운 방을 만드는 중...");
			break;
		case EFPSOnlineOperationState::Joining:
			Status = TEXT("선택한 방에 참가하는 중...");
			break;
		case EFPSOnlineOperationState::Destroying:
			Status = TEXT("세션 연결을 정리하는 중...");
			break;
		case EFPSOnlineOperationState::Starting:
			Status = TEXT("게임 시작을 준비하는 중...");
			break;
		case EFPSOnlineOperationState::Ending:
			Status = TEXT("로비 복귀를 준비하는 중...");
			break;
		default:
			break;
		}
		if (_SessionSubsystem->GetTravelState() == EFPSOnlineTravelState::Traveling)
		{
			Status = TEXT("서버에 연결하거나 레벨을 이동하는 중...");
		}
	}
	else
	{
		Status = TEXT("온라인 세션 기능을 사용할 수 없습니다.");
		_ServiceText->SetText(FText::FromString(TEXT("OFFLINE")));
	}
	if (_ReturningToMenu)
	{
		Status = TEXT("로비 메뉴로 돌아가는 중...");
	}
	_StatusText->SetText(FText::FromString(Status));
	if (Connected)
	{
		UpdateConnectedPlayers();
	}
}

void USessionMenuWidget::UpdateConnectedPlayers()
{
	FFPSOnlineSessionInfo Information;
	const bool HasInformation = _SessionSubsystem->GetCurrentSessionInfo(Information);
	_RoomInfoText->SetText(FText::FromString(HasInformation && !Information._DisplayName.IsEmpty()
		? Information._DisplayName : TEXT("데스매치 로비")));
	FString Players;
	int32 PlayerCount = 0;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (IsValid(GameState))
	{
		for (const APlayerState* Player : GameState->PlayerArray)
		{
			if (!IsValid(Player) || Player->IsInactive() || Player->IsOnlyASpectator())
			{
				continue;
			}
			++PlayerCount;
			Players += FString::Printf(TEXT("\n%02d   %s%s"), PlayerCount, *Player->GetPlayerName(),
				Player == GetOwningPlayerState() ? TEXT("  (나)") : TEXT(""));
		}
	}
	_PlayersText->SetText(FText::FromString(FString::Printf(TEXT("참가자 %d / %d\n%s"), PlayerCount,
		HasInformation ? Information._MaxPlayers : PlayerCount, *Players)));
	const bool IsHost = _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::Hosting;
	_ConnectedHintText->SetText(FText::FromString(IsHost
		? FString::Printf(TEXT("게임 시작 → %s\n게임 맵에서 전원 로딩 완료 후 설정된 카운트다운이 시작됩니다."),
			*FPackageName::GetShortName(_GameLevel.ToSoftObjectPath().GetLongPackageName()))
		: TEXT("호스트가 시작하면 같은 게임 맵으로 이동합니다.\n전원 로딩과 시작 카운트다운 동안 공격은 차단됩니다.")));
}
