// Fill out your copyright notice in the Description page of Project Settings.

#include "ParkourComponent.h"
#include "HurdleCheckComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UParkourComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	if (false == IsValid(Owner))
	{
		return;
	}

	HurdleCheckComponent = Owner->FindComponentByClass<UHurdleCheckComponent>();
}

bool UParkourComponent::TryParkour()
{
	if (bVaultActive)
	{
		return false;
	}

	if (false == IsValid(HurdleCheckComponent))
	{
		return false;
	}

	FHitResult FrontHit;

	const bool bCollision = HurdleCheckComponent->CheckFrontBlock(FrontHit);

	if (bCollision)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Front Block : %s"), *GetNameSafe(FrontHit.GetActor())));

		FHitResult TopHit;
		
		float BlockHeight = 0.0f;

		const bool bTopCollision = HurdleCheckComponent->CheckTopBlock(FrontHit, TopHit, BlockHeight);

		if (bTopCollision)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Magenta, FString::Printf(TEXT("Top Surface : Found / Height : %.1f cm"), BlockHeight));

			FHitResult BackHit;
			
			float BlockDepth = 0.0f;

			const bool bBackCollision = HurdleCheckComponent->CheckBackBlock(FrontHit, TopHit, BackHit, BlockDepth);

			if (bBackCollision)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, FString::Printf(TEXT("Back Point : Found / Depth : %.1f cm"), BlockDepth));

				FHitResult LandingHit;

				const bool bLandingCollision = HurdleCheckComponent->CheckLandingFloor(FrontHit, TopHit, BackHit, LandingHit);

				if (bLandingCollision)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow, FString::Printf(TEXT("Landing Floor : Found")));

					const bool bLandingSpace = HurdleCheckComponent->CheckLandingSpace(LandingHit);

					if (bLandingSpace)
					{
						GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Landing Space : Clear")));

						const bool bCanVaultAction = BlockHeight >= MinVaultHeight && BlockHeight <= MaxVaultHeight && BlockDepth <= MaxVaultDepth;

						if (bCanVaultAction)
						{
							PlayVault(FrontHit, LandingHit);

							if (bVaultActive)
							{
								GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Emerald, FString::Printf(TEXT("Parkour Action : Vault")));

								return true;
							}
						}

						GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Parkour Action : None")));
					}
					else
					{
						GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Landing Space : Blocked")));
					}
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Landing Floor : None")));
				}
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Back Point : None")));
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Orange, FString::Printf(TEXT("Top Surface : None")));
		}

	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Front Block : None")));
	}

	return false;
}

bool UParkourComponent::IsVaultActive() const
{
	return bVaultActive;
}

void UParkourComponent::PlayVault(const FHitResult& FrontHit, const FHitResult& LandingHit)
{
	if (bVaultActive || false == FrontHit.bBlockingHit || false == LandingHit.bBlockingHit || false == IsValid(VaultMontage.Get()))
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

	// 장애물이 앞면이 향하는 반대쪽이 vault 진행방향이 된다.
	const FVector VaultDirection = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();

	if (VaultDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation = VaultDirection.Rotation();

	// 몽타주의 VaultLand 와 검사한 착지 지점을 연결하기
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("VaultLand"), LandingHit.ImpactPoint, TargetRotation);

	const float MontageDuration = Owner->PlayAnimMontage(VaultMontage.Get());

	if (MontageDuration <= 0.f)
	{
		MotionWarping->RemoveWarpTarget(TEXT("VaultLand"));

		return;
	}

	VaultStartTransform = Owner->GetActorTransform();

	VaultBlockComponent = BlockComponent;

	// 다른 시스템이 이미 무시하고 있다면 여기서 한번 더 해제할 필요는 없으니까 하지 않음
	bVaultBlockIgnore = false == CharacterCapsule->GetMoveIgnoreComponents().Contains(BlockComponent);

	if (bVaultBlockIgnore)
	{
		CharacterCapsule->IgnoreComponentWhenMoving(BlockComponent, true);
	}

	PreviousMovementMode = CharacterMovement->MovementMode;

	PreviousCustomMovementMode = CharacterMovement->CustomMovementMode;

	// Walking 상태에서는 수직 Root Motion이 제한되니까 Vault 동안은 Flying 사용
	// => Vault 동안 보행 물리를 잠시 중단하고 몽타주의 XYZ Root Motion 적용
	CharacterMovement->SetMovementMode(MOVE_Flying);

	VaultController = Owner->GetController();

	if (VaultController.IsValid())
	{
		VaultController->SetIgnoreMoveInput(true);
	}

	bVaultActive = true;

	FOnMontageEnded EndDelegate;
	
	EndDelegate.BindUObject(this, &UParkourComponent::OnVaultEnded);

	AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultMontage.Get());
}

void UParkourComponent::OnVaultEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != VaultMontage.Get())
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (IsValid(Owner))
	{
		// 중단됐다면 장애물 충돌이 무시된 상태에서 안전한 시점으로 되돌리기
		if (bInterrupted)
		{
			Owner->SetActorTransform(VaultStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}

		UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
		if(bVaultBlockIgnore)
		{
			if (IsValid(CharacterCapsule))
			{
				if (VaultBlockComponent.IsValid())
				{
					CharacterCapsule->IgnoreComponentWhenMoving(VaultBlockComponent.Get(), false);
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
			MotionWarping->RemoveWarpTarget(TEXT("VaultLand"));
		}
	}

	if (VaultController.IsValid())
	{
		VaultController->SetIgnoreMoveInput(false);
	}

	VaultController.Reset();

	VaultBlockComponent.Reset();

	bVaultActive = false;

	bVaultBlockIgnore = false;

}
