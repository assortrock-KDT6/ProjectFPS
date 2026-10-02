#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDefines.h"
#include "LobbyPlayPanelWidget.generated.h"

class UButton;
class ULobbyStartWidget;
class USessionMenuWidget;
class UFPSOnlineSessionSubsystem;

/** 플레이 탭의 빠른 매칭과 수동 세션 창을 분리한다. */
UCLASS(Abstract)
class PROJECTFPS_API ULobbyPlayPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Session")
	void OpenSessionMenu();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CloseSessionMenu();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _LauncherPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _SessionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULobbyStartWidget> _QuickMatchWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USessionMenuWidget> _SessionMenu;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFPSOnlineSessionSubsystem> _SessionSubsystem;

	bool _SessionMenuOpen = false;

	void UpdateLauncher();

	UFUNCTION()
	void HandleOperationChanged(EFPSOnlineOperationState State);

	UFUNCTION()
	void HandleConnectionChanged(EFPSOnlineConnectionState State);

	UFUNCTION()
	void HandleTravelChanged(EFPSOnlineTravelState State);

	UFUNCTION()
	void HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage);
};
