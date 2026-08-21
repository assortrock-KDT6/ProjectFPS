// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyStartWidget.generated.h"

/**
* 로비 시작 버튼 위젯.
*
* 시작 버튼을 누르면 참가 가능한 Session을 먼저 찾고,
* 있으면 Guest로 참가하고 없으면 직접 Host가 되어 게임 레벨을 Listen Server로 연다.
* 최종 결과는 _OnAutoMatchCompleted를 통해 한 번만 전달된다.
*/
class UButton;
class UFPSOnlineSessionSubsystem;

UCLASS()
class PROJECTFPS_API ULobbyStartWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// WBP의 "StartButton" 위젯과 연결.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	// 이동할 게임 레벨 (에디터에서 드롭다운 선택). Host가 될 경우 Listen Server로 열린다.
	UPROPERTY(EditAnywhere, Category ="Travel")
	TSoftObjectPtr<UWorld> GameLevel;

	UPROPERTY(EditAnywhere, Category = "Online Session", meta = (ClampMin = "1", UIMin = "1"))
	int32 _PublicConnections = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Online Session")
	FString LastSessionError;

	/* 마지막 자동 매치에서 Host가 되었는지 여부. Guest로 참가했다면 false. */
	UPROPERTY(BlueprintReadOnly, Category = "Online Session")
	bool LastMatchIsHost = false;

protected:
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage);

	UFUNCTION()
	void HandleSessionTravelFailed(const FString& ErrorMessage);

private:
	UFPSOnlineSessionSubsystem* GetSessionSubsystem() const;
	void SetStartButtonEnabled(bool IsEnabled);
};
