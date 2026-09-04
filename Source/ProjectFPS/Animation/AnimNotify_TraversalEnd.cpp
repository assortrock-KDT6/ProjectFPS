// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_TraversalEnd.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

void UAnimNotify_TraversalEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	ACharacter* Character = (true == IsValid(MeshComp) ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr);

	UFPSCharacterMovementComponent* MovementComponent = true == IsValid(Character) 
		? Cast<UFPSCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;

	if (true == IsValid(MovementComponent))
	{
		MovementComponent->NotifyTraversalEnded();
	}
}

