#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "TitleScreenWidget.generated.h"

class UButton;

/** WBP_TitleMenu owns the layout, appearance and initial focus. */
UCLASS(Abstract)
class PROJECTFPS_API UTitleScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Title")
	void ClickStart();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void ClickQuit();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	// The Widget Blueprint supplies buttons with these names.
	UPROPERTY(BlueprintReadOnly, Category = "Title", meta = (BindWidget))
	TObjectPtr<UButton> StartMenuButton;

	UPROPERTY(BlueprintReadOnly, Category = "Title", meta = (BindWidget))
	TObjectPtr<UButton> QuitMenuButton;

	// Set to None to disable a shortcut, or handle it in Blueprint OnKeyDown.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title|Input")
	FKey StartKey = EKeys::Enter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title|Input")
	FKey QuitKey = EKeys::Escape;
};
