#include "UI/Session/SessionEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Misc/PackageName.h"

void USessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	_JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleJoinClicked);
	SetJoinAllowed(_JoinAllowed);
}

void USessionEntryWidget::NativeDestruct()
{
	_JoinButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleJoinClicked);
	Super::NativeDestruct();
}

void USessionEntryWidget::DisplaySession(const FFPSOnlineSessionInfo& Session)
{
	_ResultIndex = Session._ResultIndex;
	_HasSpace = Session._MaxPlayers > 0 && Session._CurrentPlayers < Session._MaxPlayers;
	_RoomNameText->SetText(FText::FromString(Session._DisplayName.IsEmpty() ? Session._SessionOwnerName : Session._DisplayName));
	_DetailsText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %s  ·  %s"), *Session._SessionOwnerName,
		*FPackageName::GetShortName(Session._MapName), Session._IsLan ? TEXT("LAN") : TEXT("ONLINE"))));
	_PlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Session._CurrentPlayers, Session._MaxPlayers)));
	_PingText->SetText(FText::FromString(Session._PingInMs >= 0 ? FString::Printf(TEXT("%d ms"), Session._PingInMs) : TEXT("—")));
	_JoinButtonText->SetText(FText::FromString(_HasSpace ? TEXT("참가") : TEXT("정원 초과")));
	SetJoinAllowed(_JoinAllowed);
}

void USessionEntryWidget::SetJoinAllowed(bool Allowed)
{
	_JoinAllowed = Allowed;
	if (IsValid(_JoinButton))
	{
		_JoinButton->SetIsEnabled(Allowed && _HasSpace && _ResultIndex != INDEX_NONE);
	}
}

void USessionEntryWidget::HandleJoinClicked()
{
	if (_JoinAllowed && _HasSpace && _ResultIndex != INDEX_NONE)
	{
		_OnJoinRequested.Broadcast(_ResultIndex);
	}
}
