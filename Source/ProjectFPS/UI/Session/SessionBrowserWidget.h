#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDatas.h"
#include "SessionBrowserWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UFPSOnlineSessionSubsystem;

/** UI-only adapter for the existing FPS online-session subsystem. */
UCLASS()
class PROJECTFPS_API USessionBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SessionListWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RefreshButtonWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusTextWidget;

	UPROPERTY(Transient)
	TObjectPtr<UFPSOnlineSessionSubsystem> SessionSubsystem;

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleFindSessionsCompleted(bool WasSuccessful, const TArray<FFPSOnlineSessionInfo>& Sessions, const FString& ErrorMessage);

	void SetStatus(const FText& Message) const;
	void AddSessionRow(const FFPSOnlineSessionInfo& Session) const;
};
