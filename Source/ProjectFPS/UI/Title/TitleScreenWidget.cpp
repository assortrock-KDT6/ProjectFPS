#include "UI/Title/TitleScreenWidget.h"
#include "Controller/TitlePlayerController.h"
#include "Components/Button.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void UTitleScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(StartMenuButton))
	{
		StartMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ClickStart);
	}

	if (IsValid(QuitMenuButton))
	{
		QuitMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ClickQuit);
	}
}

void UTitleScreenWidget::ClickStart()
{
	if (ATitlePlayerController* Controller = Cast<ATitlePlayerController>(GetOwningPlayer()))
	{
		Controller->StartFromTitle();
	}
}

void UTitleScreenWidget::ClickQuit()
{
	if (ATitlePlayerController* Controller = Cast<ATitlePlayerController>(GetOwningPlayer()))
	{
		Controller->QuitFromTitle();
	}
}

FReply UTitleScreenWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	// Let the Widget Blueprint handle input before applying the default shortcuts.
	FReply Reply = Super::NativeOnKeyDown(Geometry, Event);
	if (Reply.IsEventHandled())
	{
		return Reply;
	}

	if (Event.GetKey() == StartKey)
	{
		ClickStart();
		return FReply::Handled();
	}

	if (Event.GetKey() == QuitKey)
	{
		ClickQuit();
		return FReply::Handled();
	}

	return Reply;
}
