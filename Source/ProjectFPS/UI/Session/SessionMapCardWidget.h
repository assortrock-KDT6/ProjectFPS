#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDatas.h"
#include "SessionMapCardWidget.generated.h"

class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionMapSelected, int32, MapIndex);

/** Stable map identity and selection event only. The Widget Blueprint owns presentation. */
UCLASS(Abstract)
class PROJECTFPS_API USessionMapCardWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="Session") FSessionMapSelected OnMapSelected;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") FFPSPlayableMap Map;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") int32 MapIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") bool Selected = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") TObjectPtr<UTexture2D> ThumbnailTexture;
    UFUNCTION(BlueprintCallable, Category="Session") void DisplayMap(const FFPSPlayableMap& InMap, int32 InIndex, bool InSelected);
    UFUNCTION(BlueprintCallable, Category="Session") void SetSelected(bool InSelected);
    UFUNCTION(BlueprintCallable, Category="Session") void RequestSelect();
protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Session|View") void RefreshCard();
};
