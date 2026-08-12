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
private:
	UPROPERTY() 
	TObjectPtr<UUserWidget> _ShowWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> FriendsWidget;


protected:
	virtual void BeginPlay() override;

	// 로비 메인 화면
	UPROPERTY(EditAnywhere, Category = "HUD|Screens")
	TSubclassOf<UUserWidget> _LobbyWidgetClass;

	// 오버레이 창
	 
	// 상점 화면
	UPROPERTY(EditAnywhere, Category = "HUD|Overlays")
	TSubclassOf<UUserWidget> _ShopWidgetClass;
	
	// 친구창 화면
	UPROPERTY(EditAnywhere, Category = "HUD|Overlays")
	TSubclassOf<UUserWidget> _FriendsWidgetClass;

public:
	UFUNCTION(BlueprintCallable, Category = "HUD") void ToggleShop();
	UFUNCTION(BlueprintCallable, Category = "HUD") void ToggleFriends();
};
