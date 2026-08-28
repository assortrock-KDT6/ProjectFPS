// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Common/GameDefines.h"

UFPSCharacterMovementComponent::UFPSCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFPSCharacterMovementComponent::StartTravelsal()
{
	if (nullptr == CharacterOwner)
	{
		return;
	}

	SetMovementMode(MOVE_Custom, static_cast<uint8>(EProjectCustomMovementMode::Travelsal));
}

void UFPSCharacterMovementComponent::StopTravelsal()
{
	if (nullptr == CharacterOwner)
	{
		return;
	}

	/**
	 * 공중에서 Traversal이 끝나면 Falling, 
	 * 바닥이라면 Walking으로 전환
	 */

	if (true == IsMovingOnGround())
	{
		SetMovementMode(MOVE_Walking);
	}
	else
	{
		SetMovementMode(MOVE_Falling);
	}
}

bool UFPSCharacterMovementComponent::IsTraversing() const
{
	const bool Traversing = MovementMode == MOVE_Custom && 
							CustomMovementMode == static_cast<uint8>(EProjectCustomMovementMode::Travelsal);

	return Traversing;
}

void UFPSCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations) /* override */
{

}

void UFPSCharacterMovementComponent::PhysTraversal(float DeltaTime, int32 Iterations)
{
}
