#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "UI/Loading/FPSLoadingScreenData.h"
#include "FPSLoadingSubsystem.generated.h"

class UFPSLoadingWidget;
class UFPSOnlineSessionSubsystem;
class APlayerController;
struct FWorldContext;

/** Per-instance loading lifecycle. Never blocks the game thread waiting for replicated readiness. */
UCLASS(Config=Game)
class PROJECTFPS_API UFPSLoadingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
    friend struct FSessionWorkflowTestAccess;
private:
    UPROPERTY(Config)
    TSoftObjectPtr<UFPSLoadingScreenData> ScreenData;

    UPROPERTY(Transient)
    TObjectPtr<UFPSLoadingScreenData> _LoadedData;

    UPROPERTY(Transient)
    TObjectPtr<UFPSLoadingWidget> _LoadingWidget;

    UPROPERTY(Transient)
    FFPSLoadingScreenState _State;
    // Keep presentation textures available across world garbage collection.
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTexture2D>> RetainedBackgrounds;

    UPROPERTY(Transient)
    TObjectPtr<UFPSOnlineSessionSubsystem> Sessions;

private:
    TWeakObjectPtr<UWorld> ObservedWorld;

    TWeakObjectPtr<UWorld> TravelSourceWorld;

    TWeakObjectPtr<APlayerController> BlockedController;

    bool AwaitingDestinationWorld = false;

    float RefreshAccumulator = 0;

    FDelegateHandle PreLoadHandle;

    FDelegateHandle PostLoadHandle;

    FTSTicker::FDelegateHandle TickHandle;

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void Deinitialize() override;

public:
    /** Called for client travel too, including clients following a host's ServerTravel. */
    void NotifyTravelStarting(const FString& Destination);

public:
    UFUNCTION(BlueprintPure, Category="Loading") 
    FFPSLoadingScreenState GetLoadingState() const 
    { 
        return _State;
    }
    
    UFUNCTION(BlueprintPure, Category="Loading") 
    bool IsLoadingScreenVisible() const 
    { 
        return _State.Phase != EFPSLoadingPhase::Hidden;
    }

private:
    bool TickLoading(float DeltaSeconds);
    
    void HandlePreLoadMap(const FWorldContext& Context, const FString& MapName);
    
    void HandlePostLoadMap(UWorld* World);
    
    void ResolveDestination(const FString& Destination);
    
    void ShowLoadingWidget();
    
    void PresentBeforeMapLoad();
    
    void UpdateLoadingWidget(APlayerController* Controller);
    
    void SetInputBlocked(APlayerController* Controller);
    
    void ReleaseInputBlock();
    
    void HideScreen();

private:
    UFUNCTION() 
    void HandleSessionFailure(const FString& Error);
};
