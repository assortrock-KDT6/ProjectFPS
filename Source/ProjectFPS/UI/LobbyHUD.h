// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainHUD.h"
#include "LobbyHUD.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API ALobbyHUD : public AMainHUD
{
	GENERATED_BODY()
protected:
	

	// 로비 메인 화면
	UPROPERTY(EditAnywhere, Category = "HUD|Screens")
	TSubclassOf<UUserWidget> _LobbyWidgetClass;

protected:
	virtual void BeginPlay() override;
};
