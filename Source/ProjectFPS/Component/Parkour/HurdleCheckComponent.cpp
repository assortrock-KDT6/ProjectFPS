// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Parkour/HurdleCheckComponent.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"


UHurdleCheckComponent::UHurdleCheckComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	/** 
	 * Trace 유틸리티에는 RPC와 복제 상태가 없다.
	 */

	SetIsReplicatedByDefault(false);
}

bool UHurdleCheckComponent::BuildBaseQuery(FTraversalBaseQuery& OutResult) const
{
	OutResult = FTraversalBaseQuery();

	if (false == TraceFrontBlock(OutResult._FrontHit))
	{
		return false;
	}

	if (false == TraceTopBlock(OutResult._FrontHit, OutResult._TopHit, OutResult._ObstacleHeight))
	{
		return false;
	}

	// 앞면과 윗면이 완전히 다른 장애물에서 검출되는 것을 막는다.
	if (false == IsSameObstacle(OutResult._FrontHit, OutResult._TopHit))
	{
		return false;
	}

	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return false;
	}

	OutResult._Direction = FVector::VectorPlaneProject(-OutResult._FrontHit.ImpactNormal, Owner->GetActorUpVector().GetSafeNormal());

	return OutResult.IsValid();
}

bool UHurdleCheckComponent::TraceFrontBlock(FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const ACharacter* Owner = Cast<ACharacter>(GetOwner());

	const UWorld* World = GetWorld();

	if (false == IsValid(Owner) || false == IsValid(World))
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();

	if (false == IsValid(CharacterCapsule))
	{
		return false;
	}

	if (FrontCheckDistance <= 0.f || FrontCheckHeight < 0.f || FrontCheckRadius <= 0.f || FrontCheckHalfHeight < FrontCheckRadius)
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const FVector ForwardVector = FVector::VectorPlaneProject(Owner->GetActorForwardVector(), UpVector).GetSafeNormal();

	if (true == ForwardVector.IsNearlyZero())
	{
		return false;
	}

	const float CharacterHalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();

	const FVector CharacterBottom = Owner->GetActorLocation() - UpVector * CharacterHalfHeight;

	const FVector Start = CharacterBottom + UpVector * FrontCheckHeight;

	const FVector End = Start + ForwardVector * FrontCheckDistance;

	const FCollisionShape CheckShape = FCollisionShape::MakeCapsule(FrontCheckRadius, FrontCheckHalfHeight);

	const FQuat ShapeRotation = FQuat::Identity;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraversalFrontBlock), false, Owner);

	const bool Hit = World->SweepSingleByChannel(OutHit, Start, End, ShapeRotation, TraceChannel.GetValue(), CheckShape, QueryParams);

	const bool ValidHit = (true == Hit && true == OutHit.IsValidBlockingHit());

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		const FVector DebugEnd = Hit ? OutHit.Location : End;

		const FColor DebugColor = ValidHit ? FColor::Green : FColor::Red;

		DrawDebugLine(World, Start, DebugEnd, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);

		DrawDebugCapsule(World, DebugEnd, FrontCheckHalfHeight, FrontCheckRadius, FQuat::Identity, DebugColor, false, DebugLineThickness);

		if (ValidHit)
		{
			DrawDebugSphere(World, OutHit.ImpactPoint, FrontCheckHalfHeight, FrontCheckRadius, FColor::Yellow, false, DebugLifeTime); // OutHit.ImpactPoint 란 장애물과 실제로 접촉한 위치
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
#endif // ENABLE_DRAW_DEBUG

	return ValidHit;
}

bool UHurdleCheckComponent::ResolveLocalObstacle(const FVector& ObstaclePoint, const FVector& ObstacleNormal, FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	const FVector SafeNormal = ObstacleNormal.GetSafeNormal();
	UWorld* World = GetWorld();

	if (false == IsValid(World) || false == IsValid(Owner) || true == SafeNormal.IsNearlyZero() || ObstacleResolveTraceHalfDistance <= 0.f)
	{
		return false;
	}

	const FVector TraceOffset = SafeNormal * ObstacleResolveTraceHalfDistance;
	const FVector Start = ObstaclePoint + TraceOffset;
	const FVector End = ObstaclePoint - TraceOffset;

	//const FVector TraceOffset = SafeNormal * ObstacleResolveTraceHalfDistance;

	//// FrontHit의 법선 방향은 표면 바깥족이다.
	//const FVector Start = ObstaclePoint + TraceOffset;
	//const FVector End = ObstaclePoint - TraceOffset;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraversalResolveObstacle), false, Owner);
	const bool Hit = World->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel.GetValue(), QueryParams);
	const float NormalAgreement = (true == Hit ? FVector::DotProduct(OutHit.ImpactNormal.GetSafeNormal(), SafeNormal) : -1.f);

	const bool Resolved = (true == Hit) && true == OutHit.IsValidBlockingHit() && NormalAgreement >= 0.5f;

#if ENABLE_DRAW_DEBUG

	if (true == bDrawDebug)
	{
		const FVector DebugEnd = (true == Hit ? OutHit.ImpactPoint : End);
		const FColor DebugColor = (true == Resolved ? FColor::Cyan : FColor::Red);

		DrawDebugLine(World, Start, DebugEnd, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);
	}

#endif // ENABLE_DRAW_DEBUG

	return Resolved;
}

bool UHurdleCheckComponent::TraceTopBlock(const FHitResult& FrontHit, FHitResult& OutTopHit, float& OutHeight) const
{
	OutTopHit = FHitResult();

	OutHeight = 0.f;

	const ACharacter* Owner = Cast<ACharacter>(GetOwner());

	const UWorld* World = GetWorld();

	if (false == IsValid(Owner) || false == IsValid(World))
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();

	if (false == IsValid(CharacterCapsule) || false == FrontHit.IsValidBlockingHit())
	{
		return false;
	}

	if (MinTopHeight < 0.f || MaxTopTraceHeight <= MinTopTraceHeight || TopCheckInset < 0.f)
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

	if (true == IntoObstacle.IsNearlyZero())
	{
		return false;
	}

	// 장애물의 모서리에 걸리지 않도록 앞면에서 조금 안쪽으로 이동한다.
	const FVector CheckLocation = FrontHit.ImpactPoint + IntoObstacle * TopCheckInset;

	// 캐릭터 발바닥을 기준으로 최대 높이에서 최소 높이까지 검사한다.
	const FVector Start = CheckLocation + UpVector * (MaxTopHeight - FrontHitHeight);

	const FVector End = CheckLocation + UpVector * (MinTopHeight - FrontHitHeight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraversalTopBlock), false, Owner);

	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutTopHit, Start, End, TraceChannel.GetValue(), QueryParams);
	
	if (true == bCollision && true == OutTopHit.IsValidBlockingHit())
	{
		OutHeight = FVector::DotProduct(OutTopHit.ImpactPoint - CharacterBottom, UpVector);
	}

	const bool ValidHeight = OutHeight >= MinTopTraceHeight && OutHeight <= MaxTopTraceHeight;
	const bool ValidHit = true == bCollision && true == OutTopHit.IsValidBlockingHit() && true == ValidHeight;

#if ENABLE_DRAW_DEBUG

	if (true == bDrawDebug)
	{
		const FVector DebugEnd = ValidHit ? OutTopHit.ImpactPoint : End;

		const FColor DebugColor = ValidHit ? FColor::Cyan : FColor::Red;

		DrawDebugLine(World, Start, DebugEnd, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);

		if (ValidHit)
		{
			DrawDebugSphere(World, OutTopHit.ImpactPoint, DebugPointRadius, DebugPointSegments, FColor::Yellow, false, DebugLifeTime);
		}
	}

#endif //ENABLE_DRAW_DEBUG

	return ValidHit;
}

bool UHurdleCheckComponent::IsUsableFloor(const FHitResult& FloorHit) const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());

	if (false == IsValid(Owner))
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = Owner->GetCharacterMovement();

	return true == IsValid(MovementComponent) && true == FloorHit.IsValidBlockingHit() && true == MovementComponent->IsWalkable(FloorHit);
}

bool UHurdleCheckComponent::IsSameObstacle(const FHitResult& FirstHit, const FHitResult& SecondHit) const
{
	if (false == FirstHit.IsValidBlockingHit() || false == SecondHit.IsValidBlockingHit())
	{
		return false;
	}

	const UPrimitiveComponent* FirstComponent = FirstHit.GetComponent();

	const UPrimitiveComponent* SecondComponent = SecondHit.GetComponent();

	if (true == IsValid(FirstComponent) && true == IsValid(SecondComponent))
	{

		return FirstComponent == SecondComponent;

	}

	return (true == IsValid(FirstHit.GetActor())) && FirstHit.GetActor() == SecondHit.GetActor();
}

FQuat UHurdleCheckComponent::MakeCapsuleRotation(const FVector& UpVector)
{
	return FQuat::FindBetweenNormals(FVector::UpVector, UpVector.GetSafeNormal());
}

bool UHurdleCheckComponent::CheckVaultBackBlock(const FTraversalBaseQuery& BaseQuery, const FVaultTraceSettings& Settings, FHitResult& OutBackHit, float& OutDepth) const
{
	OutBackHit = FHitResult();

	OutDepth = 0.f;

	const ACharacter* Owner = Cast<ACharacter>(GetOwner());

	const UWorld* World = GetWorld();

	if (false == IsValid(Owner) || false == IsValid(World))
	{
		return false;
	}

	if (Settings._BackCheckDistance <= TopCheckInset || Settings._BackCheckHeight < 0.f)
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();
	
	const FVector IntoObstacle = BaseQuery._Direction.GetSafeNormal();

	if (true == IntoObstacle.IsNearlyZero())
	{
		return false;
	}

	const float TopHeightFromFront = FVector::DotProduct(BaseQuery._TopHit.ImpactPoint - BaseQuery._FrontHit.ImpactPoint, UpVector);

	const FVector FrontPointAtTopHeight = BaseQuery._FrontHit.ImpactPoint + UpVector * TopHeightFromFront;

	const FVector CheckLocation = FrontPointAtTopHeight - UpVector * Settings._BackCheckHeight;

	/** 
	 * 장애물 너머에서 앞면 방향으로 역추적하여 뒷면을 찾는다.
	 */
	const FVector Start = CheckLocation + IntoObstacle * Settings._BackCheckDistance;

	const FVector End = CheckLocation + IntoObstacle * TopCheckInset;

	FCollisionQueryParams  QueryParams(SCENE_QUERY_STAT(TraversalVaultBack), false, Owner);

	const bool Hit = World->LineTraceSingleByChannel(OutBackHit, Start, End, TraceChannel.GetValue(), QueryParams);

	const bool SameObstacle = (Hit && IsSameObstacle(BaseQuery._FrontHit, OutBackHit));

	if (true == SameObstacle)
	{
		OutDepth = FVector::DotProduct(OutBackHit.ImpactPoint - BaseQuery._FrontHit.ImpactPoint, IntoObstacle);
	}

	const bool ValidBack = (SameObstacle && OutDepth > KINDA_SMALL_NUMBER);

#if ENABLE_DRAW_DEBUG

	if (true == bDrawDebug)
	{
		const FVector   Debugend = Hit ? OutBackHit.ImpactPoint : End;
		const FColor    DebugColor = ValidBack ? FColor::Emerald : FColor::Red;

		DrawDebugLine(World, Start, Debugend, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);
	}

	if (true == ValidBack)
	{
		DrawDebugSphere(World, OutBackHit.ImpactPoint, DebugPointRadius, DebugPointSegments, FColor::Blue, false, DebugLifeTime);
	}

#endif // #if ENABLE_DRAW_DEBUG

	return ValidBack;
}

bool UHurdleCheckComponent::CheckVaultLandingFloor(const FTraversalBaseQuery& BaseQuery, const FHitResult& BackHit, const FVaultTraceSettings& Settings, FHitResult& OutLandingHit) const
{
	OutLandingHit = FHitResult();

	const ACharacter*	Owner = Cast<ACharacter>(GetOwner());

	const UWorld*		World = GetWorld();

	if (false == IsValid(Owner) || false == IsValid(World) 
		|| false == BaseQuery.IsValid() || false == BackHit.IsValidBlockingHit())
	{

		return false;

	}

	if (Settings._LandingForwardOffset <= 0.f
		|| Settings._LandingTraceUp <= 0.f
		|| Settings._LandingTraceDown <= 0.f)
	{

		return false;

	}

	const FVector UpVector = Owner->GetActorUpVector();

	const FVector IntoObstacle = BaseQuery._Direction.GetSafeNormal();

	if (true == IntoObstacle.IsNearlyZero())
	{
		return false;
	}

	const float BacktoTopHeight = FVector::DotProduct(BaseQuery._TopHit.ImpactPoint - BackHit.ImpactPoint, UpVector);
	
	const FVector BackPointAtTopHeight = BackHit.ImpactPoint + UpVector * BacktoTopHeight;

	const FVector CheckLocation = BackPointAtTopHeight + IntoObstacle * Settings._LandingForwardOffset;

	const FVector Start			= CheckLocation + UpVector * Settings._LandingTraceUp;

	const FVector End			= CheckLocation - UpVector * Settings._LandingTraceDown;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraversalVaultLanding), false, Owner);

	const bool Hit = World->LineTraceSingleByChannel(OutLandingHit, Start, End, TraceChannel.GetValue(), QueryParams);

	const bool UsableFloor = (true == Hit) && true == IsUsableFloor(OutLandingHit);

#if ENABLE_DRAW_DEBUG

	if (true == bDrawDebug)
	{
		const FVector DebugEnd = Hit ? OutLandingHit.ImpactPoint : End;
		const FColor DebugColor = !Hit ? FColor::Red : (UsableFloor ? FColor::Green : FColor::Orange);

		DrawDebugLine(World, Start, DebugEnd, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);
		if (true == Hit)
		{
			DrawDebugSphere(World, OutLandingHit.ImpactPoint, DebugPointRadius, DebugPointSegments, DebugColor, false, DebugLifeTime);
		}
	}

#endif

	return UsableFloor;
}

bool UHurdleCheckComponent::CheckMantleTopFloor(const FTraversalBaseQuery& BaseQuery, const FMantleTraceSettings& Settings, FHitResult& OutTopFloorHit) const
{
	OutTopFloorHit = FHitResult();

	const ACharacter* Owner = Cast<ACharacter>(GetOwner());

	const UWorld* World = GetWorld();

	if (false == IsValid(Owner) || false == IsValid(World))
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();

	if (false == IsValid(Capsule) || false == BaseQuery.IsValid() || Settings._TopFloorTraceHalfDistance <= 0.f)
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const FVector IntoObstacle = BaseQuery._Direction.GetSafeNormal();

	if (true == IntoObstacle.IsNearlyZero())
	{
		return false;
	}

	const float TopHeightFromFront = FVector::DotProduct(BaseQuery._TopHit.ImpactPoint - BaseQuery._FrontHit.ImpactPoint, UpVector);

	const FVector FrontPointAtTopHeight = BaseQuery._FrontHit.ImpactPoint + UpVector * TopHeightFromFront;

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();

	const FVector CheckLocation = FrontPointAtTopHeight + IntoObstacle * (CapsuleRadius + TopCheckInset);

	const FVector Start = CheckLocation + UpVector * Settings._TopFloorTraceHalfDistance;

	const FVector End = CheckLocation - UpVector * Settings._TopFloorTraceHalfDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraversalMantleTopFloor), false, Owner);

	const bool Hit = World->LineTraceSingleByChannel(OutTopFloorHit, Start, End, TraceChannel.GetValue(), QueryParams);

	const bool UsableFloor = (true == Hit && true == IsUsableFloor(OutTopFloorHit));

#if ENABLE_DRAW_DEBUG

	if (true == bDrawDebug)
	{
		const FVector DebugEnd = (Hit ? OutTopFloorHit.ImpactPoint : End);

		const FColor DebugColor = (false == Hit) ? FColor::Red : (UsableFloor ? FColor::Purple : FColor::Orange);
	
		DrawDebugLine(World, Start, DebugEnd, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);

		if (true == Hit)
		{
			DrawDebugSphere(World, OutTopFloorHit.ImpactPoint, DebugPointRadius, DebugPointSegments, DebugColor, false, DebugLifeTime);
		}
	}

#endif	
	return UsableFloor;
}

float UHurdleCheckComponent::GetLandingFloorClearance() const
{
	return LandingFloorClearance;
}

bool UHurdleCheckComponent::CheckLandingSpace(const FHitResult& LandingHit) const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());

	const UWorld* World = GetWorld();

	if (false == IsValid(Owner) || false == IsValid(World))
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();

	if (false == IsValid(CharacterCapsule))
	{
		return false;
	}

	if (false == IsUsableFloor(LandingHit) || LandingFloorClearance < 0.f)
	{
		return false;
	}

	const FVector UpVector = Owner->GetActorUpVector();

	const float CapsuleRadius = CharacterCapsule->GetScaledCapsuleRadius();

	const float CapsuleHalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();

	// 바닥부터 캡슐까지의 높이로 중심 구하기
	const FVector CapsuleCenter = LandingHit.ImpactPoint + UpVector * (CapsuleHalfHeight + LandingFloorClearance);

	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

	const FQuat CapsuleRatation = FQuat::Identity;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraversalLandingSpace), false, Owner);
	//QueryParams.AddIgnoredActor(Owner);

	const bool Blocked = World->OverlapBlockingTestByProfile(CapsuleCenter, CapsuleRatation, CharacterCapsule->GetCollisionProfileName(), CapsuleShape, QueryParams);

#if ENABLE_DRAW_DEBUG

	if (true == bDrawDebug)
	{
		const FColor DebugColor = (true == Blocked ? FColor::Red : FColor::Green);
		DrawDebugCapsule(World, CapsuleCenter, CapsuleHalfHeight, CapsuleRadius, CapsuleRatation, DebugColor, false, DebugLifeTime);
	}

#endif //ENABLE_DRAW_DEBUG

	return false == Blocked;

}