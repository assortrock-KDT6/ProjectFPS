// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyStartWidget.generated.h"

/**
* 로비 시작 버튼 위젯
* 시작 버튼 클릭 시 인게임으로 진입함.
 * 
 */
class UButton;

UCLASS()
class PROJECTFPS_API ULobbyStartWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;

	// WBP의 "StartButton" 위젯과 연결.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	//이동할 게임 레벨 (에디터에서 드롭다운 선택) *확인좀 해보고 변경예정.
	UPROPERTY(EditAnywhere, Category ="Travel")
	TSoftObjectPtr<UWorld> GameLevel;

protected:
	UFUNCTION()
	void OnStartCliced();


};
