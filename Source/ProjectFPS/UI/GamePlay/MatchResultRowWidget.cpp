#include "UI/GamePlay/MatchResultRowWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UMatchResultRowWidget::SetResult(const FPlayerMatchResult& Result, bool IsLocalPlayer)
{
	FString Name = Result._PlayerName;
	Name.ReplaceInline(TEXT("\n"), TEXT(" "));
	Name.ReplaceInline(TEXT("\r"), TEXT(" "));
	if (Name.IsEmpty()) Name = FString::Printf(TEXT("Player %d"), Result._PlayerId);
	const FText PlayerName = FText::FromString(Name);

	if (RankText) RankText->SetText(FText::AsNumber(Result._Rank));
	if (PlayerNameText) PlayerNameText->SetText(IsLocalPlayer
		? FText::Format(NSLOCTEXT("MatchResults", "LocalPlayerName", "{0} (나)"), PlayerName) : PlayerName);
	if (KillsText) KillsText->SetText(FText::AsNumber(Result._Stats._KillScore));
	if (DeathsText) DeathsText->SetText(FText::AsNumber(Result._Stats._DeathScore));
	if (LocalPlayerHighlight) LocalPlayerHighlight->SetVisibility(
		IsLocalPlayer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
