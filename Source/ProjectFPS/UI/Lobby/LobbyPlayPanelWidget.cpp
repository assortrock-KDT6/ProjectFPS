#include "UI/Lobby/LobbyPlayPanelWidget.h"

#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "UI/Lobby/LobbyStartWidget.h"
#include "UI/Session/SessionMapCardWidget.h"
#include "UI/Session/SessionMenuWidget.h"

void ULobbyPlayPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	_SessionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenSessionMenu);
	_SessionMenu->_OnCloseRequested.AddUniqueDynamic(this, &ThisClass::CloseSessionMenu);
	_SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>() : nullptr;
	SetIsFocusable(true);
	// Preserve the existing designer button and its user-adjusted position.
	_MapSelectButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_165")));
	if (_MapSelectButton)
	{
		_MapSelectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenMapSelection);
	}
	if (_MapSelectionCloseButton)
	{
		_MapSelectionCloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseMapSelection);
	}
	if (_MapSelectionBackdropButton)
	{
		_MapSelectionBackdropButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseMapSelection);
	}
	if (_DefaultQuickMap.Level.IsNull() && _QuickMatchWidget)
	{
		_DefaultQuickMap = _QuickMatchWidget->GetQuickMatchMap();
		if (const UImage* Thumbnail = Cast<UImage>(GetWidgetFromName(TEXT("RF_MapThumbnail"))))
		{
			_DefaultMapThumbnail = Cast<UTexture2D>(Thumbnail->GetBrush().GetResourceObject());
		}
	}
	_MapSelectionOpen = false;
	RefreshQuickMapCard();

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
	if (_MapSelectButton)
	{
		_MapSelectButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenMapSelection);
	}
	if (_MapSelectionCloseButton)
	{
		_MapSelectionCloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseMapSelection);
	}
	if (_MapSelectionBackdropButton)
	{
		_MapSelectionBackdropButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseMapSelection);
	}
	if (_MapSelectionGrid)
	{
		_MapSelectionGrid->ClearChildren();
	}
	_MapSelectionOpen = false;
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
	CloseMapSelection();
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

	const bool CanQuickMatch = CanChooseQuickMap();
	if (!CanQuickMatch)
	{
		_MapSelectionOpen = false;
	}
	_QuickMatchWidget->SetIsEnabled(CanQuickMatch && !_MapSelectionOpen);
	if (_MapSelectButton)
	{
		_MapSelectButton->SetIsEnabled(CanQuickMatch);
	}
	if (_MapSelectionOverlay)
	{
		_MapSelectionOverlay->SetVisibility(_MapSelectionOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

bool ULobbyPlayPanelWidget::CanChooseQuickMap() const
{
	return !_SessionMenuOpen && IsValid(_SessionSubsystem) && !_SessionSubsystem->IsBusy()
		&& !_SessionSubsystem->IsAutoMatchInProgress() && !_SessionSubsystem->IsExitInProgress()
		&& _SessionSubsystem->GetConnectionState() == EFPSOnlineConnectionState::None;
}

void ULobbyPlayPanelWidget::RebuildMapOptions()
{
	QuickMapOptions.Reset();
	// Keep the configured test map available even when it is outside the session catalog.
	if (!_DefaultQuickMap.Level.IsNull())
	{
		QuickMapOptions.Add(_DefaultQuickMap);
		if (QuickMapOptions[0].Thumbnail.IsNull())
		{
			QuickMapOptions[0].Thumbnail = _DefaultMapThumbnail;
		}
	}
	if (IsValid(_SessionSubsystem))
	{
		for (const FFPSPlayableMap& Map : _SessionSubsystem->GetPlayableMaps())
		{
			const bool AlreadyListed = QuickMapOptions.ContainsByPredicate([&Map](const FFPSPlayableMap& Existing)
			{
				return Existing.Mode == Map.Mode && Existing.GetMapPath() == Map.GetMapPath();
			});
			if (!AlreadyListed)
			{
				QuickMapOptions.Add(Map);
			}
		}
	}
}

void ULobbyPlayPanelWidget::OpenMapSelection()
{
	if (!CanChooseQuickMap() || !_QuickMatchWidget || !_MapSelectionOverlay || !_MapSelectionGrid || !_MapSelectionCardClass)
	{
		return;
	}
	if (_MapSelectionOpen)
	{
		CloseMapSelection();
		return;
	}
	RebuildMapOptions();
	_MapSelectionGrid->ClearChildren();
	const FFPSPlayableMap Selected = _QuickMatchWidget->GetQuickMatchMap();
	for (int32 Index = 0; Index < QuickMapOptions.Num(); ++Index)
	{
		USessionMapCardWidget* Card = CreateWidget<USessionMapCardWidget>(GetOwningPlayer(), _MapSelectionCardClass);
		if (!Card)
		{
			continue;
		}
		Card->OnMapSelected.AddUniqueDynamic(this, &ThisClass::SelectQuickMap);
		_MapSelectionGrid->AddChildToUniformGrid(Card, Index / 3, Index % 3);
		const FFPSPlayableMap& Map = QuickMapOptions[Index];
		const bool IsSelected = Map.GetMapPath() == Selected.GetMapPath() && Map.Mode == Selected.Mode;
		Card->DisplayMap(Map, Index, IsSelected);
		// The shared session card has its own palette; use this lobby's neutral selection colors.
		if (UBorder* Frame = Cast<UBorder>(Card->GetWidgetFromName(TEXT("CardFrame"))))
		{
			const float Shade = IsSelected ? 0.7f : 0.08f;
			Frame->SetBrushColor(FLinearColor(Shade, Shade, Shade, 1.f));
		}
	}
	_MapSelectionOpen = true;
	UpdateLauncher();
	if (GetOwningPlayer())
	{
		SetUserFocus(GetOwningPlayer());
	}
}

void ULobbyPlayPanelWidget::CloseMapSelection()
{
	const bool WasOpen = _MapSelectionOpen;
	_MapSelectionOpen = false;
	UpdateLauncher();
	if (WasOpen && CanChooseQuickMap() && _MapSelectButton && GetOwningPlayer())
	{
		_MapSelectButton->SetUserFocus(GetOwningPlayer());
	}
}

void ULobbyPlayPanelWidget::SelectQuickMap(int32 MapIndex)
{
	if (!_MapSelectionOpen || !CanChooseQuickMap() || !QuickMapOptions.IsValidIndex(MapIndex) || !_QuickMatchWidget)
	{
		return;
	}
	if (_QuickMatchWidget->SetQuickMatchMap(QuickMapOptions[MapIndex]))
	{
		RefreshQuickMapCard();
		CloseMapSelection();
	}
}

void ULobbyPlayPanelWidget::RefreshQuickMapCard()
{
	if (!_QuickMatchWidget)
	{
		return;
	}
	const FFPSPlayableMap Map = _QuickMatchWidget->GetQuickMatchMap();
	if (UTextBlock* Caption = Cast<UTextBlock>(GetWidgetFromName(TEXT("RF_MapCaption"))))
	{
		Caption->SetText(Map.DisplayName);
	}
	if (UImage* Thumbnail = Cast<UImage>(GetWidgetFromName(TEXT("RF_MapThumbnail"))))
	{
		UTexture2D* Texture = Map.Thumbnail.LoadSynchronous();
		if (!Texture && Map.GetMapPath() == _DefaultQuickMap.GetMapPath())
		{
			Texture = _DefaultMapThumbnail;
		}
		Thumbnail->SetBrushFromTexture(Texture);
		Thumbnail->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

FReply ULobbyPlayPanelWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (_MapSelectionOpen && Event.GetKey() == EKeys::Escape)
	{
		CloseMapSelection();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
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
