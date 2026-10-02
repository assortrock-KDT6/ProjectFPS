#pragma once
#include "CoreMinimal.h"
#include "Common/GameDatas.h"
#include "SessionMenuState.generated.h"

/** Data only. Text, colors, layout and confirmation state belong to the Widget Blueprint. */
USTRUCT(BlueprintType)
struct FSessionMenuState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool Available = false;
    UPROPERTY(BlueprintReadOnly) bool Ready = false;
    UPROPERTY(BlueprintReadOnly) bool CanBrowse = false;
    UPROPERTY(BlueprintReadOnly) bool Connected = false;
    UPROPERTY(BlueprintReadOnly) bool Host = false;
    UPROPERTY(BlueprintReadOnly) bool CanStart = false;
    UPROPERTY(BlueprintReadOnly) bool CanSelectMap = false;
    UPROPERTY(BlueprintReadOnly) bool ReturningToMenu = false;
    UPROPERTY(BlueprintReadOnly) EFPSOnlineConnectionState Connection = EFPSOnlineConnectionState::None;
    UPROPERTY(BlueprintReadOnly) EFPSOnlineOperationState Operation = EFPSOnlineOperationState::Idle;
    UPROPERTY(BlueprintReadOnly) EFPSOnlineTravelState Travel = EFPSOnlineTravelState::None;
    UPROPERTY(BlueprintReadOnly) EFPSMatchMode Mode = EFPSMatchMode::PVP;
    UPROPERTY(BlueprintReadOnly) FName Service;
    UPROPERTY(BlueprintReadOnly) FFPSOnlineSessionInfo Room;
    UPROPERTY(BlueprintReadOnly) TArray<FString> PlayerNames;
    UPROPERTY(BlueprintReadOnly) int32 LocalPlayerIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FFPSPlayableMap SelectedMap;
    UPROPERTY(BlueprintReadOnly) TArray<FFPSPlayableMap> MapOptions;
    UPROPERTY(BlueprintReadOnly) int32 SelectedMapIndex = INDEX_NONE;
};
