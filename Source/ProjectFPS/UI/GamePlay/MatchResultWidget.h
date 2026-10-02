#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "TimerManager.h"
#include "MatchResultWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UMatchResultRowWidget;

/** 확정된 경기 결과와 서버 기준 로비 복귀 카운트다운. 레이아웃은 Widget Blueprint에서 정의한다. */
UCLASS(Abstract)
class PROJECTFPS_API UMatchResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetResults(const TArray<FPlayerMatchResult>& Results, double ReturnServerTime, int32 LocalPlayerId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	UPROPERTY(EditDefaultsOnly, Category = "Match|Results")
	TSubclassOf<UMatchResultRowWidget> ResultRowWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ResultRowsBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidget))
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
