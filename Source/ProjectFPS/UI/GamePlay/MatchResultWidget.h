#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "TimerManager.h"
#include "MatchResultWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/** 확정된 경기 결과와 서버 기준 로비 복귀 카운트다운. 기본 레이아웃은 C++에서 제공한다. */
UCLASS()
class PROJECTFPS_API UMatchResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetResults(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime, int32 LocalPlayerId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 같은 이름의 위젯을 배치한 WBP로 레이아웃을 교체할 수 있다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ResultRowsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(BlueprintReadOnly, Category = "Match|Results")
	TArray<FPlayerMatchResult> _Results;

private:
	double _ReturnServerTime = 0.0;
	int32 _LocalPlayerId = INDEX_NONE;
	FTimerHandle _CountdownTimer;

	void RefreshCountdown();
	void PopulateResults();
};
