// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "GameMode/PlayerMatchStats.h"
#include "KillLogEntryWidget.generated.h"

class	UTextBlock;
class	UImage;
class	UBorder;
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
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> _Background;

	UPROPERTY(EditDefaultsOnly, Category = "KillLog|Colors")
	FLinearColor _SelfKillColor = FLinearColor(0.025f, 0.18f, 0.65f, 0.8f);

	UPROPERTY(EditDefaultsOnly, Category = "KillLog|Colors")
	FLinearColor _AllyKillColor = FLinearColor(0.025f, 0.4f, 0.09f, 0.8f);

	UPROPERTY(EditDefaultsOnly, Category = "KillLog|Colors")
	FLinearColor _EnemyKillColor = FLinearColor(0.65f, 0.035f, 0.035f, 0.8f);

	UPROPERTY(EditDefaultsOnly, Category = "KillLog|Colors")
	FLinearColor _UnknownKillColor = FLinearColor(0.08f, 0.08f, 0.08f, 0.7f);

	UPROPERTY(Transient)
	EKillLogRelation _EntryRelation = EKillLogRelation::Unknown;

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
	UFUNCTION(BlueprintPure, Category = "KillLog")
	EKillLogRelation GetEntryRelation() const { return _EntryRelation; }

	UFUNCTION(BlueprintCallable, Category = "KillLog")
	bool SetEntryInformation(const FPlayerKillLogResult& Information);
};
