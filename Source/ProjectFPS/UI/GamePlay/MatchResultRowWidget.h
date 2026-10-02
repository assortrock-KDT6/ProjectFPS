#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "MatchResultRowWidget.generated.h"

class UTextBlock;
class UBorder;

/** 한 플레이어의 결과 데이터만 갱신한다. 배치와 강조 스타일은 Widget Blueprint에서 편집한다. */
UCLASS(Abstract)
class PROJECTFPS_API UMatchResultRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetResult(const FPlayerMatchResult& Result, bool IsLocalPlayer);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DeathsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> LocalPlayerHighlight;
};
