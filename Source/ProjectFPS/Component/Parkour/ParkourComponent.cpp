#include "ParkourComponent.h"
// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Parkour/ParkourComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UParkourComponent::TryParkour()
{
	FHitResult FrontHit;

	const bool bCollision = CheckFrontBlock(FrontHit);

	if (bCollision)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Front Block : %s"), *GetNameSafe(FrontHit.GetActor())));
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

	const bool bCollision = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		TraceChannel.GetValue(),
		CheckShape,
		QueryParams);

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

bool UParkourComponent::CheckTopSurface(const FHitResult& FrontHit, FHitResult& OutHit) const
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
