#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDefines.h"
#include "Common/GameDatas.h"
#include "LobbyPlayPanelWidget.generated.h"

class UButton;
class ULobbyStartWidget;
class USessionMenuWidget;
class UFPSOnlineSessionSubsystem;
class USessionMapCardWidget;
class UUniformGridPanel;
class UTexture2D;

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

	UFUNCTION(BlueprintCallable, Category = "Lobby|Map Selection")
	void OpenMapSelection();

	UFUNCTION(BlueprintCallable, Category = "Lobby|Map Selection")
	void CloseMapSelection();

	UFUNCTION(BlueprintCallable, Category = "Lobby|Map Selection")
	void SelectQuickMap(int32 MapIndex);

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lobby|Map Selection")
	TArray<FFPSPlayableMap> QuickMapOptions;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _LauncherPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _SessionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULobbyStartWidget> _QuickMatchWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USessionMenuWidget> _SessionMenu;

	UPROPERTY(Transient)
	TObjectPtr<UButton> _MapSelectButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> _MapSelectionOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> _MapSelectionGrid;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> _MapSelectionCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> _MapSelectionBackdropButton;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby|Map Selection")
	TSubclassOf<USessionMapCardWidget> _MapSelectionCardClass;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFPSOnlineSessionSubsystem> _SessionSubsystem;

	bool _SessionMenuOpen = false;
	bool _MapSelectionOpen = false;
	FFPSPlayableMap _DefaultQuickMap;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> _DefaultMapThumbnail;

	void UpdateLauncher();
	bool CanChooseQuickMap() const;
	void RefreshQuickMapCard();
	void RebuildMapOptions();

	UFUNCTION()
	void HandleOperationChanged(EFPSOnlineOperationState State);

	UFUNCTION()
	void HandleConnectionChanged(EFPSOnlineConnectionState State);

	UFUNCTION()
	void HandleTravelChanged(EFPSOnlineTravelState State);

	UFUNCTION()
	void HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage);
};
