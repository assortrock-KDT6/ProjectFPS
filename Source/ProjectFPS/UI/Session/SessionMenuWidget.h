#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDatas.h"
#include "Common/GameDefines.h"
#include "TimerManager.h"
#include "SessionMenuWidget.generated.h"

class UButton;
class UCheckBox;
class UEditableTextBox;
class USpinBox;
class UTextBlock;
class UVerticalBox;
class USessionEntryWidget;
class UFPSOnlineSessionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSessionMenuCloseRequested);

/** 세션 작업은 Subsystem에 위임하고, 완료 이벤트와 실제 접속 상태로 UI를 갱신한다. */
UCLASS(Abstract)
class PROJECTFPS_API USessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionMenuCloseRequested _OnCloseRequested;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CloseMenu();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void ShowFindPanel();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void ShowCreatePanel();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void RefreshSessions();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void QuickJoin();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSession(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void StartMatch();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void RequestLeave();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void ConfirmLeave();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CancelLeave();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Session|Travel")
	TSoftObjectPtr<UWorld> _LobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/LobbyLevel.LobbyLevel")));

	UPROPERTY(EditDefaultsOnly, Category = "Session|Travel")
	TSoftObjectPtr<UWorld> _GameLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/TestLevel.TestLevel")));

	UPROPERTY(EditDefaultsOnly, Category = "Session", meta = (ClampMin = "2", ClampMax = "16"))
	int32 _DefaultMaxPlayers = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Session", meta = (ClampMin = "1"))
	int32 _MaxSearchResults = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	TSubclassOf<USessionEntryWidget> _SessionEntryClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _BrowsePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _ConnectedPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _ExitConfirmation;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> _RoomNameInput;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> _MaxPlayersInput;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> _AllowJoinInProgressCheck;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _CreateButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _FindTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _CreateTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _FindPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _CreatePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _FindTabIndicator;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> _CreateTabIndicator;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _RefreshButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _StartMatchButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _LeaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _ConfirmLeaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _CancelLeaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> _SessionList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _NoticeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _ServiceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _ResultCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _EmptyListText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _RoomInfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _PlayersText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _ConnectedHintText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _LeaveButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _ExitConfirmationText;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFPSOnlineSessionSubsystem> _SessionSubsystem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USessionEntryWidget>> _SessionRows;

	TArray<FFPSOnlineSessionInfo> _SearchResults;
	FTimerHandle _RefreshTimer;
	bool _InputsInitialized = false;
	bool _ExitRequested = false;
	bool _ReturningToMenu = false;
	bool _CreatingRoom = false;

	bool CanBrowse() const;
	bool IsBusy() const;
	bool BuildCreateOptions(FFPSSessionCreateOptions& Options);
	void BindSessionEvents();
	void UnbindSessionEvents();
	void UpdateDisplay();
	void ClearSearchResults();
	void SetNotice(const FString& Message, bool IsError = false);
	void ClearNotice();
	void UpdateConnectedPlayers();
	void ReturnToMenu();

	UFUNCTION()
	void HandleFindCompleted(bool WasSuccessful, const TArray<FFPSOnlineSessionInfo>& Sessions, const FString& ErrorMessage);

	UFUNCTION()
	void HandleRequestCompleted(bool WasSuccessful, const FString& ErrorMessage);

	UFUNCTION()
	void HandleExitCompleted(bool WasSuccessful, const FString& ErrorMessage);

	UFUNCTION()
	void HandleAutoMatchCompleted(bool WasSuccessful, bool IsHost, const FString& ErrorMessage);

	UFUNCTION()
	void HandleSessionError(const FString& ErrorMessage);

	UFUNCTION()
	void HandleOperationChanged(EFPSOnlineOperationState State);

	UFUNCTION()
	void HandleConnectionChanged(EFPSOnlineConnectionState State);

	UFUNCTION()
	void HandleTravelChanged(EFPSOnlineTravelState State);
};
