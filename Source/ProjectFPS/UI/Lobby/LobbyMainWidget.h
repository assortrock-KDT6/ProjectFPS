// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyMainWidget.generated.h"

/**
* 
* 로미 메인 위젯
* 탭(메뉴 위젯)의 신호를 받아, 콘텐츠 스위처의 패널을 전환한다. (버튼 입력받아서 화면 띄움.)
*/

class UWidgetSwitcher;
class ULobbyMenuWidget;

UCLASS()
class PROJECTFPS_API ULobbyMainWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 탭 내용(패널 화면)을 담아 하나씩 보여줌.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	// 상단 탭바 위젯
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULobbyMenuWidget> LobbyMenu;

protected:
	virtual void NativeOnInitialized() override;

	// 메뉴의 OnTabSelected 에 바인딩 될 핸들러.
	UFUNCTION() 
	void HandleTabSelected(int32 Index);


};
