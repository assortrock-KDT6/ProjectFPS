#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Session/SessionMenuState.h"
#include "SessionMenuWidget.generated.h"

class UFPSOnlineSessionSubsystem;
class AFPSLobbyGameState;
class AGameStateBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSessionMenuCloseRequested);

/** Validated session commands and lifecycle only; this base owns no designer widgets. */
UCLASS(Abstract)
class PROJECTFPS_API USessionMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="Session") 
    FSessionMenuCloseRequested _OnCloseRequested;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") 
    FSessionMenuState State;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") 
    TArray<FFPSOnlineSessionInfo> _SearchResults;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") 
    FString _Notice;

    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") 
    bool _NoticeIsError = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") 
    bool _HasSearchResults = false;

    UFUNCTION(BlueprintCallable, Category="Session") 
    void CloseMenu();

    UFUNCTION(BlueprintCallable, Category="Session") 
    void RefreshSessions();

    UFUNCTION(BlueprintCallable, Category="Session") 
    void CreateSession(const FString& RoomName, int32 MaxPlayers, bool AllowJoinInProgress);

    UFUNCTION(BlueprintCallable, Category="Session") 
    void QuickJoin(const FString& RoomName, int32 MaxPlayers, bool AllowJoinInProgress);

    UFUNCTION(BlueprintCallable, Category="Session") 
    void JoinSession(int32 ResultIndex);

    UFUNCTION(BlueprintCallable, Category="Session") 
    void StartMatch();

    UFUNCTION(BlueprintCallable, Category="Session") 
    void ConfirmLeave();

    UFUNCTION(BlueprintCallable, Category="Session") 
    void SelectMode(EFPSMatchMode Mode);

    UFUNCTION(BlueprintCallable, Category="Session") 
    void SelectMap(int32 MapIndex);

protected:
    UPROPERTY(EditDefaultsOnly, Category="Session", meta=(ClampMin="1")) 
    int32 _MaxSearchResults = 100;

    UFUNCTION(BlueprintImplementableEvent, Category="Session|View")     
    void RefreshView();
    UFUNCTION(BlueprintImplementableEvent, Category="Session|View")     
    void RefreshResultList();
    UFUNCTION(BlueprintImplementableEvent, Category="Session|View")     
    void ProcessCloseAction();
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
    UPROPERTY(Transient) 
    TObjectPtr<AFPSLobbyGameState> _ObservedLobby;
    
    UPROPERTY(Transient) 
    TObjectPtr<UFPSOnlineSessionSubsystem> _SessionSubsystem;
    
    FDelegateHandle _GameStateSetHandle;

    bool _ExitRequested = false;

    bool _ReturningToMenu = false;

    bool _ViewReady = false;

    EFPSMatchMode _RequestedMode = EFPSMatchMode::PVP;

    bool CanBrowse() const;
    bool IsBusy() const;
    bool BuildCreateOptions(FFPSSessionCreateOptions& Options, const FString& RoomName, int32 MaxPlayers, bool AllowJoinInProgress);
    void BindSessionEvents();
    void UnbindSessionEvents();
    void ObserveGameState(AGameStateBase* GameState);
    UFUNCTION() void UpdateDisplay();
    void ClearSearchResults();
    void SetNotice(const FString& Message, bool IsError = false);
    void ClearNotice();
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
	void HandleOperationChanged(EFPSOnlineOperationState NewState);

	UFUNCTION()
	void HandleConnectionChanged(EFPSOnlineConnectionState NewState);

	UFUNCTION()
	void HandleTravelChanged(EFPSOnlineTravelState NewState);
};
