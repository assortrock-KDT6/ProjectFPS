// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainHUD.h"
#include "Common/GameDefines.h"
#include "GameHUD.generated.h"

/**
 * 게임 레벨 HUD
 * MatchPhase에 따라 화면 전환.
 * Inventory, Item Info,  
 * 
 */
UCLASS()
class PROJECTFPS_API AGameHUD : public AMainHUD
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> _InventoryWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> _MapWidget;

protected:

	// 각 위젯별 화면 BP_GameHUD -> 디테일에서 지정.
	UPROPERTY(EditAnywhere, Category = "HUD|Screens")	
	TSubclassOf<UUserWidget> _WaitingWidgetClass;		// 대기화면

	UPROPERTY(EditAnywhere, Category = "HUD|Screens")
	TSubclassOf<UUserWidget> _GameplayWidgetClass;		// 인게임 내 화면

	UPROPERTY(EditAnywhere, Category = "HUD|Overlays")	// HUD 그룹 안에 Overlays 하위 그룹(계층)
	TSubclassOf<UUserWidget> _InventoryWidgetClass;		// 인벤토리

	UPROPERTY(EditAnywhere, Category = "HUD|Overlays")	// 지도 열기
	TSubclassOf<UUserWidget> _MapWidgetClass;

public:

	// 매치 단계에 맞는 화면으로 전환 (로비, 게임화면)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SwitchTo(EMatchPhase Phase);

	// 인벤토리 오버레이
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventory();

	// 지도 오버레이
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleMap();

	
};
