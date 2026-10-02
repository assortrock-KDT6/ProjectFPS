// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/FPSSpectatorPawn.h"
#include "GameFramework/SpectatorPawnMovement.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Controller/PlayerControllerBase.h"
#include "GameFramework/PlayerController.h"

AFPSSpectatorPawn::AFPSSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bAddDefaultMovementBindings = true;

	USpectatorPawnMovement* Movement = Cast<USpectatorPawnMovement>(GetMovementComponent());
	if (true == IsValid(Movement))
	{
		Movement->MaxSpeed = 1500.f;
		Movement->Acceleration = 4000.f;
		Movement->Deceleration = 5000.f;
	}
}
