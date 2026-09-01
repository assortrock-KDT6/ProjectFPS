// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Parkour/MantleComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

UMantleComponent::UMantleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMantleComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	if (false == IsValid(Owner))
	{
		return;
	}

	HurdleCheckComponent = Owner->FindComponentByClass<UHurdleCheckComponent>();
}

bool UMantleComponent::TryMantle()
{
	if (bMantleActive)
	{
		return false;
	}

	if (false == IsValid(HurdleCheckComponent))
	{
		return false;
	}

	FHitResult FrontHit;

	if (false == HurdleCheckComponent->TraceFrontBlock(FrontHit))
	{
		return false;
	}

	FHitResult TopHit;

	float MantleHeight = 0.0f;
	
	if (false == HurdleCheckComponent->TraceTopBlock(FrontHit, TopHit, MantleHeight))
	{
		return false;
	}

	const bool bValidHeight = MantleHeight >= MinMantleHeight && MantleHeight <= MaxMantleHeight;
	
	if (false == bValidHeight)
	{
		return false;
	}

	if (MantleHeight < Mantle2MHeight)
	{
		ActiveMantleMontage = MantleMontage1M;
	}
	else
	{
		ActiveMantleMontage = MantleMontage2M;
	}

	if (false == IsValid(ActiveMantleMontage.Get()))
	{
		return false;
	}

	FHitResult TopFloorHit;

	if (false == HurdleCheckComponent->CheckTopFloor(FrontHit, TopHit, TopFloorHit))
	{
		return false;
	}

	if (false == HurdleCheckComponent->CheckLandingSpace(TopFloorHit))
	{
		return false;
	}

	PlayMantle(FrontHit, TopFloorHit);

	return bMantleActive;
}

bool UMantleComponent::IsMantleActive() const
{
	return bMantleActive;
}

void UMantleComponent::PlayMantle(const FHitResult& FrontHit, const FHitResult& TopFloorHit)
{
	if (bMantleActive || false == FrontHit.bBlockingHit || false == TopFloorHit.bBlockingHit || false == IsValid(ActiveMantleMontage.Get()))
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());

	if (false == IsValid(Owner))
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = Owner->GetMesh();

	if (false == IsValid(CharacterMesh))
	{
		return;
	}

	UCharacterMovementComponent* CharacterMovement = Owner->GetCharacterMovement();

	if (false == IsValid(CharacterMovement))
	{
		return;
	}

	UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();

	if (false == IsValid(CharacterCapsule))
	{
		return;
	}

	UPrimitiveComponent* BlockComponent = FrontHit.GetComponent();

	if (false == IsValid(BlockComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

	if (false == IsValid(AnimInstance))
	{
		return;
	}

	UMotionWarpingComponent* MotionWarping = Owner->FindComponentByClass<UMotionWarpingComponent>();

	if (false == IsValid(MotionWarping))
	{
		return;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const FVector MantleDirection = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();

	if (MantleDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation = MantleDirection.Rotation();

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("MantleTop"), TopFloorHit.ImpactPoint, TargetRotation);

	const float MontageDuration = Owner->PlayAnimMontage(ActiveMantleMontage.Get());

	if (MontageDuration <= 0.0f)
	{
		MotionWarping->RemoveWarpTarget(TEXT("MantleTop"));

		return;
	}

	MantleStartTransform = Owner->GetActorTransform();

	MantleBlockComponent = BlockComponent;

	bMantleBlockIgnore = false == CharacterCapsule->GetMoveIgnoreComponents().Contains(BlockComponent);

	if (bMantleBlockIgnore)
	{
		CharacterCapsule->IgnoreComponentWhenMoving(BlockComponent, true);
	}

	PreviousMovementMode = CharacterMovement->MovementMode;

	PreviousCustomMovementMode = CharacterMovement->CustomMovementMode;

	CharacterMovement->SetMovementMode(MOVE_Flying);

	MantleController = Owner->GetController();

	if (MantleController.IsValid())
	{
		MantleController->SetIgnoreMoveInput(true);
	}

	bMantleActive = true;

	FOnMontageEnded EndDelegate;

	EndDelegate.BindUObject(this, &UMantleComponent::OnMantleEnded);

	AnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveMantleMontage.Get());
}

void UMantleComponent::OnMantleEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveMantleMontage.Get())
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (IsValid(Owner))
	{
		// 중단됐다면 장애물 충돌이 무시된 상태에서 안전한 시점으로 되돌리기
		if (bInterrupted)
		{
			Owner->SetActorTransform(MantleStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}

		UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
		if (bMantleBlockIgnore)
		{
			if (IsValid(CharacterCapsule))
			{
				if (MantleBlockComponent.IsValid())
				{
					CharacterCapsule->IgnoreComponentWhenMoving(MantleBlockComponent.Get(), false);
				}
			}
		}

		UCharacterMovementComponent* CharacterMovement = Owner->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->SetMovementMode(PreviousMovementMode.GetValue(), PreviousCustomMovementMode);
		}

		UMotionWarpingComponent* MotionWarping = Owner->FindComponentByClass<UMotionWarpingComponent>();
		if (IsValid(MotionWarping))
		{
			MotionWarping->RemoveWarpTarget(TEXT("MantleTop"));
		}
	}

	if (MantleController.IsValid())
	{
		MantleController->SetIgnoreMoveInput(false);
	}

	MantleController.Reset();

	MantleBlockComponent.Reset();

	ActiveMantleMontage = nullptr;

	bMantleActive = false;

	bMantleBlockIgnore = false;
}



