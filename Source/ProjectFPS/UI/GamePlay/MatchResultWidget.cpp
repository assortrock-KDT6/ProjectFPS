#include "UI/GamePlay/MatchResultWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

namespace
{
	UTextBlock* MakeResultText(UWidgetTree* Tree, const FText& Text, int32 Size, FName Name = NAME_None)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = Size;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		return Block;
	}

	void AddResultCell(UWidgetTree* Tree, UHorizontalBox* Row, const FText& Text, float Weight)
	{
		UTextBlock* Cell = MakeResultText(Tree, Text, 20);
		Cell->SetAutoWrapText(true);
		UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Cell);
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Weight;
		Slot->SetSize(Size);
		Slot->SetPadding(FMargin(10.f, 8.f));
	}
}

void UMatchResultWidget::SetResults(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime, int32 LocalPlayerId)
{
	_Results = Results;
	_Results.Sort([](const FPlayerMatchResult& A, const FPlayerMatchResult& B)
	{
		return A._Rank == B._Rank ? A._PlayerId < B._PlayerId : A._Rank < B._Rank;
	});
	_ReturnServerTime = ReturnServerTime;
	_LocalPlayerId = LocalPlayerId;
}

TSharedRef<SWidget> UMatchResultWidget::RebuildWidget()
{
	if (nullptr == WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!WidgetTree->RootWidget)
	{
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
		Backdrop->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.97f));
		Backdrop->SetHorizontalAlignment(HAlign_Center);
		Backdrop->SetVerticalAlignment(VAlign_Center);
		Backdrop->SetPadding(FMargin(24.f));
		WidgetTree->RootWidget = Backdrop;

		USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
		PanelSize->SetWidthOverride(840.f);
		PanelSize->SetHeightOverride(600.f);
		Backdrop->SetContent(PanelSize);
		UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>();
		PanelSize->SetContent(Panel);

		UTextBlock* Title = MakeResultText(WidgetTree, NSLOCTEXT("MatchResults", "Title", "경기 결과"), 36);
		Panel->AddChildToVerticalBox(Title)->SetPadding(FMargin(10.f, 0.f, 10.f, 20.f));
		SummaryText = MakeResultText(WidgetTree, FText::GetEmpty(), 22, TEXT("SummaryText"));
		Panel->AddChildToVerticalBox(SummaryText)->SetPadding(FMargin(10.f, 0.f, 10.f, 20.f));

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
		AddResultCell(WidgetTree, Header, NSLOCTEXT("MatchResults", "Rank", "순위"), 0.12f);
		AddResultCell(WidgetTree, Header, NSLOCTEXT("MatchResults", "Player", "플레이어"), 0.58f);
		AddResultCell(WidgetTree, Header, NSLOCTEXT("MatchResults", "Kills", "KILL"), 0.15f);
		AddResultCell(WidgetTree, Header, NSLOCTEXT("MatchResults", "Deaths", "DEATH"), 0.15f);
		Panel->AddChildToVerticalBox(Header);
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		Panel->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ResultRowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResultRowsBox"));
		Scroll->AddChild(ResultRowsBox);

		UTextBlock* Hint = MakeResultText(WidgetTree,
			NSLOCTEXT("MatchResults", "TieHint", "KILL 동점은 공동 순위입니다."), 16);
		Panel->AddChildToVerticalBox(Hint)->SetPadding(FMargin(10.f, 16.f));
		CountdownText = MakeResultText(WidgetTree, FText::GetEmpty(), 22, TEXT("CountdownText"));
		Panel->AddChildToVerticalBox(CountdownText)->SetPadding(FMargin(10.f, 8.f));
	}

	return Super::RebuildWidget();
}

void UMatchResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	PopulateResults();

	RefreshCountdown();

	GetWorld()->GetTimerManager().SetTimer(_CountdownTimer, this, &UMatchResultWidget::RefreshCountdown, 0.1f, true);
}

void UMatchResultWidget::PopulateResults()
{
	if (SummaryText)
	{
		SummaryText->SetText(NSLOCTEXT("MatchResults", "Finished", "경기가 종료되었습니다."));
	}
	if (!ResultRowsBox)
	{
		return;
	}

	ResultRowsBox->ClearChildren();

	for (const FPlayerMatchResult& Result : _Results)
	{
		const bool IsLocal = Result._PlayerId == _LocalPlayerId;
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetBrushColor(IsLocal ? FLinearColor(0.035f, 0.22f, 0.35f, 1.f)
			: FLinearColor(0.035f, 0.05f, 0.07f, 1.f));
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Background->SetContent(Row);
		ResultRowsBox->AddChildToVerticalBox(Background)->SetPadding(FMargin(0.f, 2.f));
		FString Name = Result._PlayerName;
		Name.ReplaceInline(TEXT("\n"), TEXT(" "));
		Name.ReplaceInline(TEXT("\r"), TEXT(" "));
		if (Name.IsEmpty()) Name = FString::Printf(TEXT("Player %d"), Result._PlayerId);
		if (IsLocal) Name += TEXT(" (나)");
		AddResultCell(WidgetTree, Row, FText::AsNumber(Result._Rank), 0.12f);
		AddResultCell(WidgetTree, Row, FText::FromString(Name), 0.58f);
		AddResultCell(WidgetTree, Row, FText::AsNumber(Result._Stats._KillScore), 0.15f);
		AddResultCell(WidgetTree, Row, FText::AsNumber(Result._Stats._DeathScore), 0.15f);
		if (IsLocal && SummaryText)
		{
			SummaryText->SetText(FText::Format(NSLOCTEXT("MatchResults", "PersonalResult", "나의 순위 {0}위  ·  KILL {1}  ·  DEATH {2}"),
				FText::AsNumber(Result._Rank), FText::AsNumber(Result._Stats._KillScore), FText::AsNumber(Result._Stats._DeathScore)));
		}
	}
}

void UMatchResultWidget::RefreshCountdown()
{
	if (!CountdownText || !GetWorld()) return;
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
