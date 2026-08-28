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
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UParkourComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	if (false == IsValid(Owner))
	{
		return;
	}

	HurdleCheckComponent	= Owner->FindComponentByClass<UHurdleCheckComponent>();
	MotionWarpingComponent	= Owner->FindComponentByClass<UMotionWarpingComponent>();
}

void UParkourComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UParkourComponent, VaultMontage);
	DOREPLIFETIME(UParkourComponent, bVaultActive);
	DOREPLIFETIME(UParkourComponent, bVaultBlockIgnore);
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
	
	if (false == HurdleCheckComponent->CheckFrontBlock(FrontHit))
	{
		return false;
	}

	FHitResult TopHit;
	
	float BlockHeight = 0.0f;
	
	if (false == HurdleCheckComponent->CheckTopBlock(FrontHit, TopHit, BlockHeight))
	{
		return false;
	}

	FHitResult BackHit;
	
	float BlockDepth = 0.0f;
	
	if (false == HurdleCheckComponent->CheckBackBlock(FrontHit, TopHit, BackHit, BlockDepth))
	{
		return false;
	}

	FHitResult LandingHit;

	if (false == HurdleCheckComponent->CheckLandingFloor(FrontHit, TopHit, BackHit, LandingHit))
	{
		return false;
	}

	if (false == HurdleCheckComponent->CheckLandingSpace(LandingHit))
	{
		return false;
	}

	const bool bCanVaultAction = BlockHeight >= MinVaultHeight && BlockHeight <= MaxVaultHeight && BlockDepth <= MaxVaultDepth;

	if (false == bCanVaultAction)
	{
		return false;
	}

	PlayVault(FrontHit, LandingHit);

	if (bVaultActive)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green	, FString::Printf(TEXT("Front Block : %s"), *GetNameSafe(FrontHit.GetActor())));

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Magenta	, FString::Printf(TEXT("Top Surface : Found / Height : %.1f cm"), BlockHeight));

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue	, FString::Printf(TEXT("Back Point : Found / Depth : %.1f cm"), BlockDepth));

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow	, FString::Printf(TEXT("Landing Floor : Found")));

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green	, FString::Printf(TEXT("Landing Space : Clear")));

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Emerald	, FString::Printf(TEXT("Parkour Action : Vault")));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Front Block : None")));
	}

	return bVaultActive;
}

bool UParkourComponent::IsVaultActive() const
{
	return bVaultActive;
}

void UParkourComponent::Server_TryParkour_Implementation()
{
	TryParkour();
}

void UParkourComponent::Multicast_PlayValut_Implementation(FName WarpTargetName, const FVector Location, const FRotator Rotation)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return;
	}

	if (Owner->HasAuthority())
	{
		return;
	}

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, Location, Rotation);

	float MontageDuration = PlayValutMontage();

	if (MontageDuration <= 0.f)
	{
		MotionWarpingComponent->RemoveWarpTarget(WarpTargetName);

		return;
	}
}

float UParkourComponent::PlayValutMontage()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return 0.f;
	}
	return Owner->PlayAnimMontage(VaultMontage.Get());
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

	if (false == IsValid(MotionWarpingComponent))
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
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("VaultLand"), LandingHit.ImpactPoint, TargetRotation);

	float MontageDuration = PlayValutMontage();

	if (MontageDuration <= 0.f)
	{
		MotionWarpingComponent->RemoveWarpTarget(TEXT("VaultLand"));

		return;
	}
	
	Multicast_PlayValut(TEXT("VaultLand"), LandingHit.ImpactPoint, TargetRotation);

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
