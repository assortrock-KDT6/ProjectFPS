// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "FPSSpectatorPawn.generated.h"

/**
 * 
 */

class	UEnhancedInputLocalPlayerSubsystem;
class	UInputMappingContext;
class	UInputAction;
struct  FInputActionValue;


UCLASS()
class PROJECTFPS_API AFPSSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()
public:
	AFPSSpectatorPawn();
	
};
