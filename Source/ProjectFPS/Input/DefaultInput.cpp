// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/DefaultInput.h"
#include "InputMappingContext.h"
#include "InputAction.h"

UDefaultInput::UDefaultInput()
{
	ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultInputMappingContext(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/Blueprints/Input/IMC_DefaultInput.IMC_DefaultInput'"));
	if (DefaultInputMappingContext.Succeeded())
	{
		_DefaultInputMappingContext = DefaultInputMappingContext.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> MoveAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Move.IA_Move'"));
	if (MoveAction.Succeeded())
	{
		_Move = MoveAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> JumpAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Jump.IA_Jump'"));
	if (JumpAction.Succeeded())
	{
		_Jump = JumpAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> MouseLookAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_MouseLook.IA_MouseLook'"));
	if (MouseLookAction.Succeeded())
	{
		_MouseLook = MouseLookAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> MouseZoomAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Zoom.IA_Zoom'"));
	if (MouseZoomAction.Succeeded())
	{
		_MouseZoom = MouseZoomAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> ParkourAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Parkour.IA_Parkour'"));
	if (ParkourAction.Succeeded())
	{
		_Parkour = ParkourAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> InventoryAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Inventory.IA_Inventory'"));
	if (InventoryAction.Succeeded())
	{
		_Inventory = InventoryAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> MapAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Map.IA_Map'"));
	if (MapAction.Succeeded())
	{
		_Map = MapAction.Object;
	}

	ConstructorHelpers::FObjectFinder<UInputAction> InteractAction(TEXT("/Script/EnhancedInput.InputAction'/Game/Blueprints/Input/Actions/IA_Interact.IA_Interact'"));
	if (InteractAction.Succeeded())
	{
		_Interact = InteractAction.Object;
	}


}
