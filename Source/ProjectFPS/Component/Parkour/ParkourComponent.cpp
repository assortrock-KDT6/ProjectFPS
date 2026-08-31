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
#include "GameFramework/GameStateBase.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Component/Parkour/MantleComponent.h"
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
}

bool UParkourComponent::TryParkour()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 판정 실패 : Owner가 ACharacter가 아닙니다."));
		return false;
	}

	const ENetRole LocalRole = Owner->GetLocalRole();
	
	const bool CanRunTraversal = Owner->HasAuthority() || ROLE_AutonomousProxy == LocalRole;
	if (false == CanRunTraversal)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 판정 실패 : 서버 권한이 없습니다."));
		return false;
	}

	UFPSCharacterMovementComponent* CharacterMovement = Cast<UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
	if (false == IsValid(CharacterMovement))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 판정 실패 : 플레이어 전용 Movement Component가 아닙니다."));
		return false;
	}

	// if (bVaultActive)
	if (CharacterMovement->IsTraversing())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 판정 실패 : 이미 Traversal 중입니다."));
		return false;
	}

	if (false == IsValid(HurdleCheckComponent))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 판정 실패 : HurdleCheckComponent가 없습니다."));
		return false;
	}

	FHitResult FrontHit;
	
	if (false == HurdleCheckComponent->CheckFrontBlock(FrontHit))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 불가 : 전방 장애물을 찾지 못했습니다."));
		return false;
	}

	FHitResult TopHit;
	
	float BlockHeight = 0.0f;
	
	if (false == HurdleCheckComponent->CheckTopBlock(FrontHit, TopHit, BlockHeight))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 불가 : 장애물 윗면을 찾지 못했습니다."));
		return false;
	}

	FHitResult BackHit;
	
	float BlockDepth = 0.0f;
	
	if (false == HurdleCheckComponent->CheckBackBlock(FrontHit, TopHit, BackHit, BlockDepth))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 불가 : 장애물 뒷면을 찾지 못했습니다."));
		return false;
	}

	FHitResult LandingHit;

	if (false == HurdleCheckComponent->CheckLandingFloor(FrontHit, TopHit, BackHit, LandingHit))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 불가 : 착지 바닥을 찾지 못했습니다."));
		return false;
	}

	if (false == HurdleCheckComponent->CheckLandingSpace(LandingHit))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 불가 : 착지 위치의 캡슐 공간이 막혀있습니다."));
		return false;
	}

	const bool bCanVaultAction = BlockHeight >= MinVaultHeight && BlockHeight <= MaxVaultHeight && BlockDepth <= MaxVaultDepth;

	if (false == bCanVaultAction)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Vault 불가 : 높이 또는 깊이가 기존 Vault 허용 범위를 벗어났습니다."));
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
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Traversal 요청 실패 : Owner가 ACharacter가 아닙니다."));
		return;
	}

	if (false == Owner->HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Traversal 요청 실패 : 서버 권한이 없습니다."));
		return;
	}

	if (true == TryParkour())
	{
		return;
	}

	UMantleComponent* Mantle = Owner->FindComponentByClass<UMantleComponent>();
	if (false == IsValid(Mantle))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, TEXT("Mantle 판정 실패 : UMantleComponent가 없습니다."));
		return;
	}

	Mantle->TryMantle();

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

void UParkourComponent::ApplyLocalVaultCollisionIgnore(const FC_TraversalData& Data)
{
	if (true == bVaultBlockIgnore && true == LocalVaultBlockComponent.IsValid())
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner) || false == IsValid(HurdleCheckComponent))
	{
		return;
	}

	UCapsuleComponent* CharacterCapsuleComponent = Owner->GetCapsuleComponent();

	if (false == IsValid(CharacterCapsuleComponent))
	{
		return;
	}

	/**
	 * 서버는 PlayVault()에서 이미 저장했다.
	 * 클라이언트는 복제된 충돌점/법선으로 자신의 로컬 컴포넌트를 찾는다.
	 */
	if (false == LocalVaultBlockComponent.IsValid())
	{
		FHitResult LocalObstacleHit;
		if (true == HurdleCheckComponent->ResolveLocalObstacle(Data._ObstaclePoint, Data._ObstacleNormal, LocalObstacleHit))
		{
			LocalVaultBlockComponent = LocalObstacleHit.GetComponent();
		}
	}

	if (false == LocalVaultBlockComponent.IsValid())
	{
		return;
	}

	bVaultBlockIgnore = false == CharacterCapsuleComponent->GetMoveIgnoreComponents().Contains(LocalVaultBlockComponent.Get());
	if (true == bVaultBlockIgnore)
	{
		CharacterCapsuleComponent->IgnoreComponentWhenMoving(LocalVaultBlockComponent.Get(), true);
	}
}

void UParkourComponent::ClearLocalVaultCollisionIgnore()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (true == IsValid(Owner) && true == bVaultBlockIgnore
		&& true == LocalVaultBlockComponent.IsValid())
	{
		UCapsuleComponent* CharacterCapsuleComponent = Owner->GetCapsuleComponent();
		if (true == IsValid(CharacterCapsuleComponent))
		{
			CharacterCapsuleComponent->IgnoreComponentWhenMoving(LocalVaultBlockComponent.Get(), false);
		}
	}
	LocalVaultBlockComponent.Reset();
	bVaultBlockIgnore = false;
}

float UParkourComponent::GetTraversalPresentationPosition(const FC_TraversalData& Data) const
{
	const UWorld* World = GetWorld();

	if (false == IsValid(World))
	{
		return 0.f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	if (false == IsValid(GameState) || Data._ServerStartTimeSeconds <= 0.f)
	{
		return 0.f;
	}

	const float Elapsed = GameState->GetServerWorldTimeSeconds() - Data._ServerStartTimeSeconds;

	return FMath::Clamp(Elapsed, 0.f, Data._ExpectedDuration);
}

/*  Data 작성과 서버 이동 시작만 담당한다. */
void UParkourComponent::PlayVault(const FHitResult& FrontHit, const FHitResult& LandingHit)
{
	if (true == bVaultActive)
	{
		return;
	}

	if (false == FrontHit.bBlockingHit
		|| false == LandingHit.bBlockingHit)
	{
		return;
	}

	if(false == IsValid(VaultMontage.Get()))
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return;
	}

	UFPSCharacterMovementComponent* CharacterMovement = Cast< UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
	if (false == IsValid(CharacterMovement))
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = Owner->GetMesh();
	if (false == IsValid(CharacterMesh))
	{
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (false == IsValid(AnimInstance))
	{
		return;
	}

	UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
	if (false == IsValid(CharacterCapsule))
	{
		return;
	}

	if (false == IsValid(MotionWarpingComponent))
	{
		return;
	}

	UPrimitiveComponent* BlockComponent = FrontHit.GetComponent();
	if (false == IsValid(BlockComponent))
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
	
	FC_TraversalData TraversalData;
	TraversalData._Mode = EProjectCustomMovementMode::Vault;
	TraversalData._Variant = 0;
	TraversalData._ActionId = NextTraversalActionId++;

	if (0 == NextTraversalActionId)
	{
		NextTraversalActionId = 1;
	}

	TraversalData._StartLocation = Owner->GetActorLocation();

	const float LandingFloorClearance = HurdleCheckComponent->GetLandingFloorClearance();
	TraversalData._TargetLocation = LandingHit.ImpactPoint + UpVector * LandingFloorClearance;
	TraversalData._ObstaclePoint = FrontHit.ImpactPoint;
	TraversalData._ObstacleNormal = FrontHit.ImpactNormal;
	TraversalData._TargetRotation = VaultDirection.Rotation();
	TraversalData._WarpTargetName = TEXT("VaultLand");
	TraversalData._ExpectedDuration = VaultMontage->GetPlayLength() / FMath::Max(KINDA_SMALL_NUMBER, VaultMontage->RateScale);

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (true == IsValid(GameState))
	{
		TraversalData._ServerStartTimeSeconds = GameState->GetServerWorldTimeSeconds();
	}

	if (false == TraversalData.IsValid())
	{
		return;
	}

	/**
	 * 장애물 포인터는 서버의 기존 런타임 멤버에만 둔다. 
	 */

	LocalVaultBlockComponent = BlockComponent;
	VaultStartTransform = Owner->GetActorTransform();

	if (false == CharacterMovement->StartTravelsal(TraversalData))
	{
		LocalVaultBlockComponent.Reset();
	}

	//const FRotator TargetRotation = VaultDirection.Rotation();

	//// 몽타주의 VaultLand 와 검사한 착지 지점을 연결하기
	//MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("VaultLand"), LandingHit.ImpactPoint, TargetRotation);

	//float MontageDuration = PlayValutMontage();

	//if (MontageDuration <= 0.f)
	//{
	//	MotionWarpingComponent->RemoveWarpTarget(TEXT("VaultLand"));

	//	return;
	//}
	//
	//// 다른 시스템이 이미 무시하고 있다면 여기서 한번 더 해제할 필요는 없으니까 하지 않음
	//bVaultBlockIgnore = false == CharacterCapsule->GetMoveIgnoreComponents().Contains(BlockComponent);

	//if (bVaultBlockIgnore)
	//{
	//	CharacterCapsule->IgnoreComponentWhenMoving(BlockComponent, true);
	//}

	//PreviousMovementMode = CharacterMovement->MovementMode;

	//PreviousCustomMovementMode = CharacterMovement->CustomMovementMode;

	//// Walking 상태에서는 수직 Root Motion이 제한되니까 Vault 동안은 Flying 사용
	//// => Vault 동안 보행 물리를 잠시 중단하고 몽타주의 XYZ Root Motion 적용
	//CharacterMovement->SetMovementMode(MOVE_Flying);

	//VaultController = Owner->GetController();

	//if (VaultController.IsValid())
	//{
	//	VaultController->SetIgnoreMoveInput(true);
	//}

	//bVaultActive = true;

	//FOnMontageEnded EndDelegate;
	//
	//EndDelegate.BindUObject(this, &UParkourComponent::OnVaultEnded);

	//AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultMontage.Get());
}

void UParkourComponent::OnVaultEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != VaultMontage.Get())
	{
		return;
	}

	if (false == bVaultActive)
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());

	if (false == IsValid(Owner))
	{
		ExitValutPresentation();
		return;
	}
	
	UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
	if (false == IsValid(Movement))
	{
		ExitValutPresentation();
		return;
	}

	if (true == bInterrupted && true == Owner->HasAuthority())
	{
		Owner->SetActorTransform(VaultStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (true == Owner->IsLocallyControlled())
	{
		Movement->RequestFinishTraversal();
	}

	//if (true == IsValid(Owner))
	//{
	//	// 중단됐다면 장애물 충돌이 무시된 상태에서 안전한 시점으로 되돌리기
	//	if (true == bInterrupted && true == Owner->HasAuthority())
	//	{
	//		Owner->SetActorTransform(VaultStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	//	}
	//	
	//	UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
	//	if (true == IsValid(Movement))
	//	{
	//		Movement->RequestFinishTraversal();
	//	}

	//	//UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
	//	//if(bVaultBlockIgnore)
	//	//{
	//	//	if (IsValid(CharacterCapsule))
	//	//	{
	//	//		if (VaultBlockComponent.IsValid())
	//	//		{
	//	//			CharacterCapsule->IgnoreComponentWhenMoving(VaultBlockComponent.Get(), false);
	//	//		}
	//	//	}
	//	//}

	//	//UCharacterMovementComponent* CharacterMovement = Owner->GetCharacterMovement();
	//	//if (IsValid(CharacterMovement))
	//	//{
	//	//	CharacterMovement->SetMovementMode(PreviousMovementMode.GetValue(), PreviousCustomMovementMode);
	//	//}

	//	//UMotionWarpingComponent* MotionWarping = Owner->FindComponentByClass<UMotionWarpingComponent>();
	//	//if (IsValid(MotionWarping))
	//	//{
	//	//	MotionWarping->RemoveWarpTarget(TEXT("VaultLand"));
	//	//}
	//}
	//
	////if (VaultController.IsValid())
	////{
	////	VaultController->SetIgnoreMoveInput(false);
	////}

	////VaultController.Reset();

	////VaultBlockComponent.Reset();

	////bVaultActive = false;

	////bVaultBlockIgnore = false;

	//ExitValutPresentation();
}

void UParkourComponent::EnterVaultPresentation(const FC_TraversalData& Data)
{
	if (EProjectCustomMovementMode::Vault != Data._Mode || false == Data.IsValid())
	{
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner) || false == IsValid(MotionWarpingComponent) || false == IsValid(VaultMontage.Get()))
	{
		return;
	}

	/**
	 * 이미 끝난 액션이면 되살리지 않는다. 
	 */
	if (0 != Data._ActionId && Data._ActionId == FinishedTraversalActionId)
	{
		return;
	}

	/**
	 * 재생 위치가 이미 끝에 도달한 액션도 시작하지 않는다.
	 */
	if (GetTraversalPresentationPosition(Data) >= Data._ExpectedDuration - KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Authoritytarget = FVector(Data._TargetLocation);
	const FVector TargetDelta = Authoritytarget - PresentedTraversalTarget;

	const bool SameActionId = PresentedTraversalActionId == Data._ActionId;

	const bool MatchesPredictedTarget = true == Owner->IsLocallyControlled()
		&& FVector::DistSquared(PresentedTraversalTarget, FVector(Data._TargetLocation)) <= FMath::Square(TraversalConfirmationTolerance);

	/**
	 * 서버 타깃이 중간에 도착하는 소유 클라이언트인지 판별한다.
	 * Listen Server의 로컬 플레이어는 권위가 있으므로 제외한다.
	 */

	/**
	 * 소유 클라이언트가 이미 예측 실행한 Vault와 서버에서 복제된 Vault가 같은 액션이라면
	 * Montage를 다시 시작하지 않는다.
	 */
	if (true == bVaultActive && true == (SameActionId || MatchesPredictedTarget))
	{
		const bool OwnigPredictedClient = true == Owner->IsLocallyControlled() && false == Owner->HasAuthority();
		/**
		 * Autonomous Client는 이미 예측 타깃을 기준으로 Root Motion을 소비하고 있으므로 서버 확인이 도착했다고 
		 * 타깃을 즉시 바꾸지 않는다.
		 * 서버와 Simulated Proxy는 서버 타깃을 그대로 사용한다.
		 */

		if (true == OwnigPredictedClient && 0 != PresentedTraversalActionId && false == PresentedTraversalTarget.IsNearlyZero())
		{
			const FVector PredictedTarget =
				PresentedTraversalTarget;

			const float ForwardTargetError =
				FVector::DotProduct(
					TargetDelta,
					Owner->GetActorForwardVector());

			if (nullptr != GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					8.f,
					FColor::Cyan,
					FString::Printf(
						TEXT(
							"[Client Confirmation]\n"
							"Action: %u\n"
							"Predicted: %s\n"
							"Authority: %s\n"
							"Forward: %.3f cm\n"
							"Z: %.3f cm\n"
							"Total: %.3f cm"),
						Data._ActionId,
						*PredictedTarget.ToCompactString(),
						*Authoritytarget.ToCompactString(),
						ForwardTargetError,
						TargetDelta.Z,
						TargetDelta.Size()));
			}
		}

		if (false == OwnigPredictedClient)
		{
			if (NAME_None != ActiveVaultWarpTargetName && ActiveVaultWarpTargetName != Data._WarpTargetName)
			{
				MotionWarpingComponent->RemoveWarpTarget(ActiveVaultWarpTargetName);
			}
			/**
			 * 서버의 권위 있는 Target으로 갱신한다.
			 */
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(Data._WarpTargetName, Data._TargetLocation, Data._TargetRotation);

			ActiveVaultWarpTargetName = Data._WarpTargetName;
			PresentedTraversalTarget = Data._TargetLocation;
		}
		/**
		 * 클라이언트에서 예측한 ActionId 대신 서버 ActionId를 기억한다.
		 * Warp Target은 유지하더라도 ActionId 확인은 완료해야 한다.
		 */

		PresentedTraversalActionId = Data._ActionId;
		return;
	}


	/**
	 * 현재 재생 중인 Vault와 다른 액션이다.
	 * 이전 몽타주, 충돌 무시, 입력 잠금, Warp Target을 먼저 정리한다.
	 */

	if (true == bVaultActive)
	{
		ExitValutPresentation();
	}

	USkeletalMeshComponent* CharacterMesh = Owner->GetMesh();
	if (false == IsValid(CharacterMesh))
	{
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (false == IsValid(AnimInstance))
	{
		return;
	}

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
		Data._WarpTargetName,
		Data._TargetLocation,
		Data._TargetRotation);

	ActiveVaultWarpTargetName = Data._WarpTargetName;

	// Root Motion의 첫 이동 프레임 전에 적용한다.
	ApplyLocalVaultCollisionIgnore(Data);

	const float MontageDuration = PlayValutMontage();

	if (MontageDuration <= 0.f)
	{
		ClearLocalVaultCollisionIgnore();

		MotionWarpingComponent->RemoveWarpTarget(ActiveVaultWarpTargetName);
		
		ActiveVaultWarpTargetName = NAME_None;

		if (true == Owner->HasAuthority())
		{
			UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
			if (true == IsValid(Movement))
			{
				Movement->StopTravelsal();
			}
		}
		return;
	}

	bVaultActive = true;
	PresentedTraversalActionId = Data._ActionId;
	PresentedTraversalTarget = Data._TargetLocation;

	if (true == Owner->IsLocallyControlled())
	{
		VaultController = Owner->GetController();
		if (true == VaultController.IsValid())
		{
			VaultController->SetIgnoreMoveInput(true);
		}
	}
	
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UParkourComponent::OnVaultEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultMontage.Get());

	const float MaxStartPosition = FMath::Max(0.f, MontageDuration - KINDA_SMALL_NUMBER);
	const float StartPosition = FMath::Clamp(GetTraversalPresentationPosition(Data), 0.f, MaxStartPosition);

	AnimInstance->Montage_SetPosition(VaultMontage.Get(), StartPosition);
}

/**
 * Mesh 또는 AnimInstance가 없더라도 충돌과 입력은 반드시 복구되어야 하므로 중간 return을 두지 않는다.
 * 함수는 여러 번 호출해도 안전한 형태로 만든다.
 */
void UParkourComponent::ExitValutPresentation()
{
	if (0 != PresentedTraversalActionId)
	{
		FinishedTraversalActionId = PresentedTraversalActionId;
	}

	bVaultActive = false;
	PresentedTraversalActionId = 0;
	PresentedTraversalTarget = FVector::ZeroVector;

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (true == IsValid(Owner) && true == IsValid(VaultMontage.Get()))
	{
		USkeletalMeshComponent* CharacterMesh = Owner->GetMesh();

		if (true == IsValid(CharacterMesh))
		{
			UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

			if (true == AnimInstance->Montage_IsPlaying(VaultMontage.Get()))
			{
				AnimInstance->Montage_Stop(VaultMontageStopBlendTime, VaultMontage.Get());
			}
		}
	}

	// HasAuthority 조건을 두지 않는다.
	ClearLocalVaultCollisionIgnore();

	if (true == IsValid(MotionWarpingComponent) && NAME_None != ActiveVaultWarpTargetName)
	{
		MotionWarpingComponent->RemoveWarpTarget(ActiveVaultWarpTargetName);
	}

	ActiveVaultWarpTargetName = NAME_None;

	if (true == VaultController.IsValid())
	{
		VaultController->SetIgnoreMoveInput(false);
	}

	VaultController.Reset();
}