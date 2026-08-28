// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UFPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UFPSCharacterMovementComponent();

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

public:
	void StartTravelsal();
	void StopTravelsal();
	bool IsTraversing() const;

private:
	void PhysTraversal(float DeltaTime, int32 Iterations);
};
