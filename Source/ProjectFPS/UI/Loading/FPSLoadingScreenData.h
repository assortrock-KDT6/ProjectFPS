#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSLoadingScreenData.generated.h"

class UFPSLoadingWidget;
class UTexture2D;

UENUM(BlueprintType)
enum class EFPSLoadingPhase : uint8
{
    Hidden,
    LoadingMap,
    PreparingPlayer,
    WaitingForPlayers,
    Countdown
};

/** Presentation data only. Readiness and travel decisions stay in the subsystem. */
USTRUCT(BlueprintType)
struct FFPSLoadingScreenState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    EFPSLoadingPhase Phase = EFPSLoadingPhase::Hidden;

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    FText MapName;

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    FText ModeName;

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    FText StatusText;

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    TObjectPtr<UTexture2D> Background = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    int32 ReadyPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    int32 ExpectedPlayers = 0;
    UPROPERTY(BlueprintReadOnly, Category="Loading") 
    int32 CountdownSeconds = 0;
};

/** Palette/background for the single editable UMG loading view. */
UCLASS(BlueprintType)
class PROJECTFPS_API UFPSLoadingScreenData : public UDataAsset
{
    GENERATED_BODY()
public:
    // Serialized name retained for existing data assets; this class now handles every loading phase.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading", meta=(DisplayName="Loading Widget Class")) 
    TSubclassOf<UFPSLoadingWidget> WaitingWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading") 
    TSoftObjectPtr<UTexture2D> LobbyBackground;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading") 
    FLinearColor SurfaceColor = FLinearColor(0.008f,0.012f,0.018f,0.94f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading") 
    FLinearColor AccentColor = FLinearColor(0.32f,0.78f,0.86f,1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading") 
    FLinearColor TextColor = FLinearColor(0.88f,0.92f,0.95f,1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading") 
    FLinearColor MutedColor = FLinearColor(0.40f,0.48f,0.54f,1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading", meta=(ClampMin="0", ClampMax="1")) 
    float BackgroundDim = 0.42f;
};
