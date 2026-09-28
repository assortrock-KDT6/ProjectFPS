#include "UI/Session/SessionBrowserWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"

void USessionBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SessionListWidget = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("SessionList")));
	RefreshButtonWidget = Cast<UButton>(WidgetTree->FindWidget(TEXT("RefreshButton")));
	StatusTextWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StatusText")));

	if (RefreshButtonWidget)
	{
		RefreshButtonWidget->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionSubsystem = GameInstance->GetSubsystem<UFPSOnlineSessionSubsystem>();
	}

	if (SessionSubsystem)
	{
		SessionSubsystem->_OnFindSessionCompleted.AddUniqueDynamic(this, &ThisClass::HandleFindSessionsCompleted);
		SetStatus(FText::FromString(TEXT("Ready")));
	}
	else
	{
		SetStatus(FText::FromString(TEXT("Online Session subsystem is unavailable.")));
	}
}

void USessionBrowserWidget::NativeDestruct()
{
	if (RefreshButtonWidget)
	{
		RefreshButtonWidget->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (SessionSubsystem)
	{
		SessionSubsystem->_OnFindSessionCompleted.RemoveDynamic(this, &ThisClass::HandleFindSessionsCompleted);
	}

	Super::NativeDestruct();
}

void USessionBrowserWidget::HandleRefreshClicked()
{
	if (!SessionSubsystem)
	{
		SetStatus(FText::FromString(TEXT("Online Session subsystem is unavailable.")));
		return;
	}

	if (SessionListWidget)
	{
		SessionListWidget->ClearChildren();
	}

	SetStatus(FText::FromString(TEXT("Searching...")));
	SessionSubsystem->FindSessions();
}

void USessionBrowserWidget::HandleFindSessionsCompleted(
	bool WasSuccessful,
	const TArray<FFPSOnlineSessionInfo>& Sessions,
	const FString& ErrorMessage)
{
	if (!WasSuccessful)
	{
		SetStatus(FText::FromString(ErrorMessage.IsEmpty() ? TEXT("Session search failed.") : ErrorMessage));
		return;
	}

	if (Sessions.IsEmpty())
	{
		SetStatus(FText::FromString(TEXT("No sessions found.")));
		return;
	}

	for (const FFPSOnlineSessionInfo& Session : Sessions)
	{
		AddSessionRow(Session);
	}

	SetStatus(FText::Format(
		NSLOCTEXT("SessionBrowser", "SessionsFound", "{0} session(s) found."),
		FText::AsNumber(Sessions.Num())));
}

void USessionBrowserWidget::SetStatus(const FText& Message) const
{
	if (StatusTextWidget)
	{
		StatusTextWidget->SetText(Message);
	}
}

void USessionBrowserWidget::AddSessionRow(const FFPSOnlineSessionInfo& Session) const
{
	if (!SessionListWidget || !WidgetTree)
	{
		return;
	}

	UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	const FString DisplayName = Session._DisplayName.IsEmpty() ? Session._SessionOwnerName : Session._DisplayName;
	Row->SetText(FText::FromString(FString::Printf(
		TEXT("%s | %s | %d/%d | %d ms"),
		*DisplayName,
		*Session._MapName,
		Session._CurrentPlayers,
		Session._MaxPlayers,
		Session._PingInMs)));
	SessionListWidget->AddChildToVerticalBox(Row);
}
