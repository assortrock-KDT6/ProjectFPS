#include "UI/GamePlay/MatchResultWidget.h"
#include "UI/GamePlay/MatchResultRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Controller/PlayerControllerBase.h"
#include "InputCoreTypes.h"
void UMatchResultWidget::SetResults(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime, int32 LocalPlayerId)
{
	_Results = Results;
	_Results.Sort([](const FPlayerMatchResult& A, const FPlayerMatchResult& B)
	{
		return A._Rank == B._Rank ? A._PlayerId < B._PlayerId : A._Rank < B._Rank;
	});
	_ReturnServerTime = ReturnServerTime;
	_LocalPlayerId = LocalPlayerId;

	if (GetCachedWidget().IsValid())
	{
		PopulateResults();
		RefreshCountdown();
	}
}

void UMatchResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	PopulateResults();

	RefreshCountdown();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(_CountdownTimer, this, &UMatchResultWidget::RefreshCountdown, 0.1f, true);
	}
}

void UMatchResultWidget::PopulateResults()
{
	if (SummaryText)
	{
		SummaryText->SetText(NSLOCTEXT("MatchResults", "Finished", "경기가 종료되었습니다."));
	}
	if (!ResultRowsBox) return;
	ResultRowsBox->ClearChildren();

	if (!ensureMsgf(ResultRowWidgetClass && !ResultRowWidgetClass->HasAnyClassFlags(CLASS_Abstract),
		TEXT("Match result Widget Blueprint must specify a concrete ResultRowWidgetClass."))) return;

	for (const FPlayerMatchResult& Result : _Results)
	{
		const bool IsLocal = Result._PlayerId == _LocalPlayerId;
		if (auto* Row = CreateWidget<UMatchResultRowWidget>(this, ResultRowWidgetClass))
		{
			Row->SetResult(Result, IsLocal);
			ResultRowsBox->AddChildToVerticalBox(Row);
		}
		if (IsLocal && SummaryText)
		{
			SummaryText->SetText(FText::Format(NSLOCTEXT("MatchResults", "PersonalResult", "나의 순위 {0}위  ·  KILL {1}  ·  DEATH {2}"),
				FText::AsNumber(Result._Rank), FText::AsNumber(Result._Stats._KillScore), FText::AsNumber(Result._Stats._DeathScore)));
		}
	}
}
void UMatchResultWidget::RefreshCountdown()
{
	if (!CountdownText || !GetWorld())
	{
		return;
	}
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState) return;
	const int32 Seconds = FMath::Max(0, FMath::CeilToInt(_ReturnServerTime - GameState->GetServerWorldTimeSeconds()));
	CountdownText->SetText(Seconds > 0
		? FText::Format(NSLOCTEXT("MatchResults", "Countdown", "{0}초 후 로비로 이동합니다."), FText::AsNumber(Seconds))
		: NSLOCTEXT("MatchResults", "Returning", "로비로 이동 중…"));
}

void UMatchResultWidget::NativeDestruct()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(_CountdownTimer);
	Super::NativeDestruct();
}

FReply UMatchResultWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		if (!Event.IsRepeat())
			if (auto* PC = Cast<APlayerControllerBase>(GetOwningPlayer())) PC->ToggleExitMenu();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
