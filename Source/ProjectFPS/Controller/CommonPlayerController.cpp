#include "Controller/CommonPlayerController.h"

#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "UI/MainHUD.h"

void ACommonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::ToggleExitMenu);
}

void ACommonPlayerController::ToggleExitMenu()
{
	if (IsLocalController())
	{
		if (auto* HUD = Cast<AMainHUD>(GetHUD()))
		{
			HUD->ToggleExitMenu();
		}
	}
}
