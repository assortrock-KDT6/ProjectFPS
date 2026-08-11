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

public:

	// 매치 단계에 맞는 화면으로 전환 (로비, 게임화면)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SwitchTo(EMatchPhase Phase);

	// 인벤토리 오버레이
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventory();

protected:
	// 각 위젯별 화면 BP_GameHUD -> 디테일에서 지정.



	UPROPERTY(EditAnywhere, Category = "HUD|Overlays") // HUD 그룹 안에 Overlays 하위 그룹(계층)
	TSubclassOf<UUserWidget> _InventoryWidgetClass;



};
