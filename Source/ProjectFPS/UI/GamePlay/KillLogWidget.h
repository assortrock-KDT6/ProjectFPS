// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "KillLogWidget.generated.h"


class	UVerticalBox;
class	UKillLogEntryWidget;
class	AFPSGameState;
/**
 *
 */
UCLASS()
class PROJECTFPS_API UKillLogWidget : public UUserWidget
{
	GENERATED_BODY()

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> _KillLogBox;

	UPROPERTY(EditDefaultsOnly, Category = "KillLog")
	TSubclassOf<UKillLogEntryWidget> _KillLogEntryClass;

private:
	TWeakObjectPtr<AFPSGameState>	_GameState;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "KillLog|Add")
	void AddKillLog(const FPlayerKillLogResult& Result);
};
