// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "KillLogEntryWidget.generated.h"

class	UTextBlock;
class	UImage;
class	UWidgetAnimation;
struct	FPlayerKillLogResult;

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UKillLogEntryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock>		_KillerPlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock>		_KilledPlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage>			_KillWeaponImage;

	UPROPERTY(EditDefaultsOnly, Category = "KillLog", meta = (ClampMin = "0.0", Units = "s"))
	float _DisplayDuration = 5.0f;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> _FadeIn;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> _FadeOut;

	FTimerHandle _ExpireTimer;
	bool _bExpiring = false;

	void BeginFadeOut();

	UFUNCTION()
	void HandleFadeOutFinished();

public:
	bool SetEntryInformation(const FPlayerKillLogResult& Information);
};
