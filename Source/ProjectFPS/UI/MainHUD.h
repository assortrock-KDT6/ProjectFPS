// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UExitMenuWidget;

/**
 * 모든 HUD의 공통베이스 (직접 사용x, 상속 전용)
 * 위젯 생성/제거, 오버레이 토글, CurrentScreen 관리, 입력 모드 전환 담당.
 */

UCLASS()
class PROJECTFPS_API AMainHUD : public AHUD
{
	GENERATED_BODY()
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void RestoreInputAfterExitMenu();

protected:

	// 현재 선택되어 화면에 출력되고 있는 위젯
	UPROPERTY()
	TObjectPtr<UUserWidget> _CurrentScreen;

	// 공용 오버레이 - 환경설정(로비, 게임)
	UPROPERTY(EditAnywhere, Category = "HUD|Overlays")
	TSubclassOf<UUserWidget> SettingWidgetClass;
	UPROPERTY()
	TObjectPtr<UUserWidget> SettingWidget;

	UPROPERTY(Transient)
	TObjectPtr<UExitMenuWidget> ExitMenuWidget;

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleExitMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void CloseExitMenu();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsExitMenuOpen() const;

protected:
	// 각 HUD 블루프린트에서 자체적으로 메뉴를 생성하며, C++ 클래스 지정이나 버튼 바인딩은 사용하지 않는다.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	UExitMenuWidget* CreateExitMenu();

protected:
	// 선택된 위젯 제거 후 새 화면에 위젯 생성 및 표시
	UUserWidget* ShowScreen(TSubclassOf<UUserWidget> ScreenClass);

	// 현재 화면 위젯 제거 
	void RemoveCurrentScreen();

	// 오버레이 토글 : 있으면 제거, 없으면 생성 HUD위에 얹음. (위젯만, 참조, 레이어 순서 Z축)
	bool ToggleOverlay(TSubclassOf<UUserWidget> OverlayClass, TObjectPtr<UUserWidget>& OverlayPtr, int32 ZOrder = 10);

	// UI 조작용 커서와 게임중에는 안나오게 구분 
	virtual void ApplyInputMode(bool bUIMode);

	bool IsLocalHUD() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleSettings();	// 로비, 게임 둘다 상속.

protected:
	void CloseOverlay(TObjectPtr<UUserWidget>& OverlayPtr);

};
