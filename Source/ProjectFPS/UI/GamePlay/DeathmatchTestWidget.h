#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "TimerManager.h"
#include "DeathmatchTestWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;
class AFPSGameState;
class APlayerStateBase;

/** 순위표의 한 행. 배치는 WBP_DeathmatchScoreRow에서 편집한다. */
UCLASS(Abstract)
class PROJECTFPS_API UDeathmatchScoreRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void DisplayPlayer(const FPlayerMatchResult& Result, bool IsLocalPlayer, const FText& Status);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deathmatch|Appearance")
    FLinearColor _LocalRowColor = FLinearColor::White;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deathmatch|Appearance")
    FLinearColor _OtherRowColor = FLinearColor::White;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> _RowBackground;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _RankText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _KillScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _DeathScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _PlayerStatusText;
};

/** 복제된 GameState/PlayerState를 읽는 순위표. 타이머와 표시 조건은 별도 위젯 BP에서 관리한다. */
UCLASS(Abstract)
class PROJECTFPS_API UDeathmatchTestWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deathmatch", meta=(ClampMin="0"))
    float _RefreshInterval = 0.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deathmatch|Appearance")
    FLinearColor _DeadStateColor = FLinearColor::White;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deathmatch|Appearance")
    FLinearColor _ActiveStateColor = FLinearColor::White;
	UPROPERTY(EditDefaultsOnly, Category = "Deathmatch")
	TSubclassOf<UDeathmatchScoreRowWidget> _ScoreRowWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _MatchStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _LocalStatsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _LocalStateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _ScoreboardTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _MatchHintText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> _ScoreRowsBox;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	FTimerHandle _RefreshTimer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDeathmatchScoreRowWidget>> _ScoreRows;

	void RefreshDisplay();
	void DisplayScoreboard(const AFPSGameState* GameState, const APlayerStateBase* LocalPlayerState);
	void BuildLiveResults(const AFPSGameState* GameState, TArray<FPlayerMatchResult>& Results) const;
	FText GetPlayerStatus(const AFPSGameState* GameState, int32 PlayerId) const;
	static bool ComparePlayerResults(const FPlayerMatchResult& First, const FPlayerMatchResult& Second);
};
