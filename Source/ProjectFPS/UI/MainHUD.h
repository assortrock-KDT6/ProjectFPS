// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

/**
 * 모든 HUD의 공통베이스 (직접 사용x, 상속 전용)
 * 위젯 생성/제거, 오버레이 토글, CurrentScreen 관리, 입력 모드 전환 담당.
 */

UCLASS()
class PROJECTFPS_API AMainHUD : public AHUD
{
	GENERATED_BODY()

protected:

	// 현재 선택되어 화면에 출력되고 있는 위젯
	UPROPERTY()
	TObjectPtr<UUserWidget> _CurrentScreen;

	// 공용 오버레이 - 환경설정(로비, 게임)
	UPROPERTY(EditAnywhere, Category = "HUD|Overlays")
	TSubclassOf<UUserWidget> SettingWidgetClass;
	UPROPERTY()
	TObjectPtr<UUserWidget> SettingWidget;


protected:
	// 선택된 위젯 제거 후 새 화면에 위젯 생성 및 표시
	UUserWidget* ShowScreen(TSubclassOf<UUserWidget> ScreenClass);

	// 현재 화면 위젯 제거 
	void RemoveCurrentScreen();

	// 오버레이 토글 : 있으면 제거, 없으면 생성 HUD위에 얹음. (위젯만, 참조, 레이어 순서 Z축)
	void ToggleOverlay(TSubclassOf<UUserWidget> OverlayClass, TObjectPtr<UUserWidget>& OverlayPtr, int32 ZOrder = 10);

	// UI 조작용 커서와 게임중에는 안나오게 구분 
	void ApplyInputMode(bool bUIMode);

	bool IsLocalHUD() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleSettings();	// 로비, 게임 둘다 상속.


	
	
};
