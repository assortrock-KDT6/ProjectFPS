// Fill out your copyright notice in the Description page of Project Settings.

#include "ParkourComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UParkourComponent::TryParkour()
{
	if (bVaultActive)
	{
		return;
	}

	FHitResult FrontHit;

	const bool bCollision = CheckFrontBlock(FrontHit);

	if (bCollision)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Front Block : %s"), *GetNameSafe(FrontHit.GetActor())));

		FHitResult TopHit;
		
		float BlockHeight = 0.0f;

		const bool bTopCollision = CheckTopBlock(FrontHit, TopHit, BlockHeight);

		if (bTopCollision)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Magenta, FString::Printf(TEXT("Top Surface : Found / Height : %.1f cm"), BlockHeight));

			FHitResult BackHit;
			
			float BlockDepth = 0.0f;

			const bool bBackCollision = CheckBackBlock(FrontHit, TopHit, BackHit, BlockDepth);

			if (bBackCollision)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, FString::Printf(TEXT("Back Point : Found / Depth : %.1f cm"), BlockDepth));

				FHitResult LandingHit;

				const bool bLandingCollision = CheckLandingFloor(FrontHit, TopHit, BackHit, LandingHit);

				if (bLandingCollision)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow, FString::Printf(TEXT("Landing Floor : Found")));

					const bool bLandingSpace = CheckLandingSpace(LandingHit);

					if (bLandingSpace)
					{
						GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Landing Space : Clear")));

						const bool bCanVaultAction = BlockHeight >= MinVaultHeight && BlockHeight <= MaxVaultHeight && BlockDepth <= MaxVaultDepth;

						if (bCanVaultAction)
						{
							GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Emerald, FString::Printf(TEXT("Parkour Action : Vault")));

							PlayVault(FrontHit, LandingHit);
						}
						else
						{
							GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Parkour Action : None")));
						}
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
}

bool UParkourComponent::CheckFrontBlock(FHitResult& OutHit) const
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
	if (false == IsValid(CharacterCapsule))
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const FVector ForwardVector = Owner->GetActorForwardVector();

	const float CharacterHalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();

	const FVector CharacterBottom = Owner->GetActorLocation() - UpVector * CharacterHalfHeight;

	const FVector Start = CharacterBottom + UpVector * FrontCheckHeight;

	const FVector End = Start + ForwardVector * FrontCheckDistance;

	const FCollisionShape CheckShape = FCollisionShape::MakeCapsule(FrontCheckRadius, FrontCheckHalfHeight);

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(Owner);

	if (nullptr == GetWorld())	// 혹시 몰라서 체크한번하기
	{
		return false;
	}

	const bool bCollision = GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, TraceChannel.GetValue(), CheckShape, QueryParams);

	if (bDrawDebug)
	{
		const FVector DebugEnd = bCollision ? OutHit.Location : End;

		const FColor DebugColor = bCollision ? FColor::Green : FColor::Red;

		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);

		DrawDebugCapsule(GetWorld(), DebugEnd, FrontCheckHalfHeight, FrontCheckRadius, FQuat::Identity, DebugColor, false, 2.0f);

		if (bCollision)
		{
			DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 8.0f, 12, FColor::Yellow, false, 2.0f); // OutHit.ImpactPoint 란 장애물과 실제로 접촉한 위치
			// DrawDebugSphere(
			//	 GetWorld(),          // 구체를 그릴 월드
			//	 OutHit.ImpactPoint,  // 장애물과 실제로 접촉한 위치
			//	 8.0f,                // 구체 반지름 : 8cm
			//	 12,                  // 구체를 구성하는 선분 수
			//	 FColor::Yellow,      // 노란색
			//	 false,               // 영구적으로 남기지 않음
			//	 2.0f);               // 2초 후 사라짐
		}
	}

	return bCollision;
}

bool UParkourComponent::CheckTopBlock(const FHitResult& FrontHit, FHitResult& OutHit, float& OutHeight) const
{
	OutHeight = 0.0f;

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
	if (false == IsValid(CharacterCapsule))
	{
		return false;
	}

	if (false == FrontHit.bBlockingHit)
	{
		return false;
	}

	if (MaxParkourHeight <= MinParkourHeight)
	{
		return false;
	}

	if (nullptr == GetWorld())
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const float CharacterHalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();

	const FVector CharacterBottom = Owner->GetActorLocation() - UpVector * CharacterHalfHeight;

	// 앞면 충돌점이 캐릭터 발바닥보다 얼마나 높은지 계산
	const float FrontHitHeight = FVector::DotProduct(FrontHit.ImpactPoint - CharacterBottom, UpVector);

	// ImpactNormal의 반대쪽이 장애물 안쪽 방향
	const FVector IntoObstacle = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();

	if (IntoObstacle.IsNearlyZero())
	{
		return false;
	}

	// 장애물의 모서리에 걸리지 않도록 앞면에서 조금 안쪽으로 이동한다.
	const FVector CheckLocation = FrontHit.ImpactPoint + IntoObstacle * TopCheckInset;

	// 캐릭터 발바닥을 기준으로 최대 높이에서 최소 높이까지 검사한다.
	const FVector Start = CheckLocation + UpVector * (MaxParkourHeight - FrontHitHeight);

	const FVector End = CheckLocation + UpVector * (MinParkourHeight - FrontHitHeight);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel.GetValue(), QueryParams);

	if (bCollision)
	{
		OutHeight = FVector::DotProduct(OutHit.ImpactPoint - CharacterBottom, UpVector);
	}

	if (bDrawDebug)
	{
		const FVector DebugEnd = bCollision ? OutHit.ImpactPoint : End;
		const FColor DebugColor = bCollision ? FColor::Cyan : FColor::Red;

		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);

		if (bCollision)
		{
			DrawDebugSphere( GetWorld(), OutHit.ImpactPoint, 8.0f, 12, FColor::Yellow, false, 2.0f);
		}
	}

	return bCollision;
}

bool UParkourComponent::CheckBackBlock(const FHitResult& FrontHit, FHitResult& TopHit, FHitResult& OutBackHit, float& OutDepth) const
{
	OutDepth = 0.0f;

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return false;
	}

	if (false == FrontHit.bBlockingHit || false == TopHit.bBlockingHit)		// 앞에서도 위에서도 맞는게 없으면 굳이 뒤에서 검사할 필요가 X
	{
		return false;
	}

	if (BackCheckDistance <= 0.0f)
	{
		return false;
	}

	if (nullptr == GetWorld())
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	// Front ImpactNormal 의 반대 방향이 장애물 안쪽의 방향. 
	// - 를 붙이면 반대(안쪽으로)로감 --> ImpactPoint는 라인트레이스의 충돌지점이고 Normal은 해당 면이 수직으로 바라보는 방향 이걸로 장애물의 기울기를 얻을 수 있다.
	// VectorPlaneProjet() : 주어진 벡터값을 평면위에 투영하는 함수 UpVector(여기선 평면의 법선 방향)를 통해 x,z 평면에 투영한다.
	// GetSafeNormal()     : 투영한 벡터의 길이를 1로 만든다. 사용하는 이유는 다음 계산에서 정확히 TopCheckInset 만큼 이동하기 위해서 
	const FVector IntoBlock = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal(); 

	if (IntoBlock.IsNearlyZero())	// IsNearZero : 유효한 수평 방향인지 0의 근사값으로 판단하기 위해서 사용
	{
		return false;
	}

	// 앞면 충돌점과 같은 수평 위치를 윗면 높이까지 올린다.
	const float TopHeightFromFront = FVector::DotProduct(TopHit.ImpactPoint - FrontHit.ImpactPoint, UpVector);

	const FVector FrontPointAtTopHeight = FrontHit.ImpactPoint + UpVector * TopHeightFromFront;

	// 윗면보다 조금 아래에서 뒷면을 검사한다. 
	const FVector CheckLocation = FrontPointAtTopHeight - UpVector * BackCheckHeight;
	
	// 장애물 너머에서 시작하여 앞면 방향으로 역방향 Line Trace 를 쏜다.
	const FVector Start = CheckLocation + IntoBlock * BackCheckDistance;

	const FVector End = CheckLocation + IntoBlock * TopCheckInset;
	
	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(Owner);

	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutBackHit, Start, End, TraceChannel.GetValue(), QueryParams);
	
	// 앞면과 뒷면이 같은 장애물인지 확인
	const bool bBackPoint = bCollision && OutBackHit.GetActor() == FrontHit.GetActor();

	if (bBackPoint)
	{
		// 앞면부터 뒷면까지 진행 방향으로 떨어진 거리를 구한다.
		OutDepth = FVector::DotProduct(OutBackHit.ImpactPoint - FrontHit.ImpactPoint, IntoBlock);
	}
	
	if (bDrawDebug)
	{
		const FVector DebugEnd = bCollision ? OutBackHit.ImpactPoint : End;

		const FColor DebugColor = bBackPoint ? FColor::Emerald : FColor::Red;

		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);

		if (bBackPoint)
		{
			DrawDebugSphere(GetWorld(), OutBackHit.ImpactPoint, 8.0f, 12, FColor::Blue, false, 2.0f);
		}
	}
	
	return bBackPoint;
}

bool UParkourComponent::CheckLandingFloor(const FHitResult& FrontHit, const FHitResult& TopHit, const FHitResult& BackHit, FHitResult& OutHit) const
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return false;
	}

	if (false == FrontHit.bBlockingHit || false == TopHit.bBlockingHit || false == BackHit.bBlockingHit)
	{
		return false;
	}

	if (nullptr == GetWorld())
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	// 앞면에서 장애물 뒤쪽으로 수평 방향
	const FVector IntoBlock = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();

	if (IntoBlock.IsNearlyZero())
	{
		return false;
	}


	// BackHit 를 장애물 윗면 높이까지 올린 위치
	const float BackToTopHeight = FVector::DotProduct(TopHit.ImpactPoint - BackHit.ImpactPoint, UpVector);

	const FVector BackTopPoint = BackHit.ImpactPoint + UpVector * BackToTopHeight;

	// 뒷면에서 캐릭터 캡슐 콜라이더보다 조금 앞 위치
	const FVector CheckLocation = BackTopPoint + IntoBlock * LandingCheckFront;

	const FVector Start = CheckLocation + UpVector * LandingCheckUp;

	const FVector End = CheckLocation - UpVector * LandingCheckDown;

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(Owner);

	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel.GetValue(), QueryParams);

	if (bDrawDebug)
	{
		const FVector DebugEnd = bCollision ? OutHit.ImpactPoint : End;

		const FColor DebugColor = bCollision ? FColor::Yellow : FColor::Red;

		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);

		if (bCollision)
		{
			DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 8.0f, 12, FColor::Orange, false, 2.0f);
		}
	}

	return bCollision;
}

bool UParkourComponent::CheckLandingSpace(const FHitResult& LandingHit) const
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
	if (false == IsValid(CharacterCapsule))
	{
		return false;
	}

	if (false == LandingHit.bBlockingHit)
	{
		return false;
	}

	if (nullptr == GetWorld())
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const float CapsuleRadius = CharacterCapsule->GetScaledCapsuleRadius();

	const float CapsuleHalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();

	// 바닥부터 캡슐까지의 높이로 중심 구하기
	const FVector CapsuleCenter = LandingHit.ImpactPoint + UpVector * (CapsuleHalfHeight + LandingCapsule);

	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(Owner);

	// 캐릭터 캡슐과 같은 충돌 프로필로 공간을 검사하고 막힌게 있다면 착지 공간을 사용할 수 없고 막힌게 없으면 착지할 공간이 사용가능
	const bool bBlocked = GetWorld()->OverlapBlockingTestByProfile(CapsuleCenter, FQuat::Identity, CharacterCapsule->GetCollisionProfileName(), CapsuleShape, QueryParams);

	if (bDrawDebug)
	{
		const FColor DebugColor = bBlocked ? FColor::Red : FColor::Green;

		DrawDebugCapsule(GetWorld(), CapsuleCenter, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, DebugColor, false, 2.0f);
	}

	return false == bBlocked;

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
