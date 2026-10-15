#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CommonPlayerController.generated.h"

/** Shared menu controls for the title and gameplay controllers. */
UCLASS(Abstract)
class PROJECTFPS_API ACommonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ToggleExitMenu();

protected:
	virtual void SetupInputComponent() override;
};
