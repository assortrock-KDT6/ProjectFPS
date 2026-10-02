#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Loading/FPSLoadingScreenData.h"
#include "FPSLoadingWidget.generated.h"

/** No designer widgets or travel logic in this base; Blueprint owns the view. */
UCLASS(Abstract)
class PROJECTFPS_API UFPSLoadingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Transient, Category="Loading") 
    FFPSLoadingScreenState State;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Loading") 
    TObjectPtr<UFPSLoadingScreenData> Style;

    UFUNCTION(BlueprintCallable, Category="Loading|View")
    void ApplyLoadingState(const FFPSLoadingScreenState& InState, UFPSLoadingScreenData* InStyle);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Loading|View") 
    void RefreshLoadingView();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
};
