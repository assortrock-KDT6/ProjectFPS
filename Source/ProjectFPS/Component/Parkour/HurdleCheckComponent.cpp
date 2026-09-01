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
	if (IsSameObstacle(OutResult._FrontHit, OutResult._TopHit))
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

#endif

	// QueryParams.AddIgnoredActor(Owner);

	return Resolved;
}

bool UHurdleCheckComponent::TraceTopBlock(const FHitResult& FrontHit, FHitResult& OutTopHit, float& OutHeight) const
{
	OutTopHit = FHitResult();
	OutHeight = 0.0f;

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
		const FVector DebugEnd = bCollision ? OutTopHit.ImpactPoint : End;
		const FColor DebugColor = bCollision ? FColor::Cyan : FColor::Red;

		DrawDebugLine(World, Start, DebugEnd, DebugColor, false, DebugLifeTime, 0, DebugLineThickness);

		if (bCollision)
		{
			DrawDebugSphere(World, OutTopHit.ImpactPoint, DebugPointRadius, DebugPointSegments, FColor::Yellow, false, DebugLifeTime);
		}
	}

#endif //ENABLE_DRAW_DEBUG

	return bCollision;
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

bool UHurdleCheckComponent::CheckVaultBackBlock(const const FTraversalBaseQuery& BaseQuery, const FVaultTraceSettings& Settings, FHitResult& OutBackHit, float& OutDepth) const
{
	OutBackHit = FHitResult();
	OutDepth = 0.0f;

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

	const FVector CheckLocation = BackPointAtTopHeight + IntoObstacle * Settings._LandingTraceUp;

	const FVector Start			= CheckLocation + UpVector * Settings._LandingTraceDown;

	const FVector End = CheckLocation - UpVector * Settings._LandingTraceDown;

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

//bool UHurdleCheckComponent::CheckVaultBackBlock(const FHitResult& FrontHit, FHitResult& TopHit, FHitResult& OutBackHit, float& OutDepth) const
//{
//	OutBackHit = FHitResult();
//	OutDepth = 0.0f;
//
//	ACharacter* Owner = Cast<ACharacter>(GetOwner());
//	UWorld*		World = GetWorld();
//
//	if (false == IsValid(Owner) || false == IsValid(World))
//	{
//		return false;
//	}
//
//	if (false == FrontHit.bBlockingHit || false == TopHit.bBlockingHit)		// 앞에서도 위에서도 맞는게 없으면 굳이 뒤에서 검사할 필요가 X
//	{
//		return false;
//	}
//
//	if (BackCheckDistance <= 0.0f)
//	{
//		return false;
//	}
//
//	if (nullptr == GetWorld())
//	{
//		return false;
//	}
//
//	const FVector UpVector = Owner->GetActorUpVector();
//
//	// Front ImpactNormal 의 반대 방향이 장애물 안쪽의 방향. 
//	// - 를 붙이면 반대(안쪽으로)로감 --> ImpactPoint는 라인트레이스의 충돌지점이고 Normal은 해당 면이 수직으로 바라보는 방향 이걸로 장애물의 기울기를 얻을 수 있다.
//	// VectorPlaneProjet() : 주어진 벡터값을 평면위에 투영하는 함수 UpVector(여기선 평면의 법선 방향)를 통해 x,z 평면에 투영한다.
//	// GetSafeNormal()     : 투영한 벡터의 길이를 1로 만든다. 사용하는 이유는 다음 계산에서 정확히 TopCheckInset 만큼 이동하기 위해서 
//	const FVector IntoBlock = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();
//
//	if (IntoBlock.IsNearlyZero())	// IsNearZero : 유효한 수평 방향인지 0의 근사값으로 판단하기 위해서 사용
//	{
//		return false;
//	}
//
//	// 앞면 충돌점과 같은 수평 위치를 윗면 높이까지 올린다.
//	const float TopHeightFromFront = FVector::DotProduct(TopHit.ImpactPoint - FrontHit.ImpactPoint, UpVector);
//
//	const FVector FrontPointAtTopHeight = FrontHit.ImpactPoint + UpVector * TopHeightFromFront;
//
//	// 윗면보다 조금 아래에서 뒷면을 검사한다. 
//	const FVector CheckLocation = FrontPointAtTopHeight - UpVector * BackCheckHeight;
//
//	// 장애물 너머에서 시작하여 앞면 방향으로 역방향 Line Trace 를 쏜다.
//	const FVector Start = CheckLocation + IntoBlock * BackCheckDistance;
//
//	const FVector End = CheckLocation + IntoBlock * TopCheckInset;
//
//	FCollisionQueryParams QueryParams;
//
//	QueryParams.AddIgnoredActor(Owner);
//
//	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutBackHit, Start, End, TraceChannel.GetValue(), QueryParams);
//
//	// 앞면과 뒷면이 같은 장애물인지 확인
//	const bool bBackPoint = bCollision && OutBackHit.GetActor() == FrontHit.GetActor();
//
//	if (bBackPoint)
//	{
//		// 앞면부터 뒷면까지 진행 방향으로 떨어진 거리를 구한다.
//		OutDepth = FVector::DotProduct(OutBackHit.ImpactPoint - FrontHit.ImpactPoint, IntoBlock);
//	}
//
//	if (bDrawDebug)
//	{
//		const FVector DebugEnd = bCollision ? OutBackHit.ImpactPoint : End;
//
//		const FColor DebugColor = bBackPoint ? FColor::Emerald : FColor::Red;
//
//		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);
//
//		if (bBackPoint)
//		{
//			DrawDebugSphere(GetWorld(), OutBackHit.ImpactPoint, 8.0f, 12, FColor::Blue, false, 2.0f);
//		}
//	}
//
//	return bBackPoint;
//}

//bool UHurdleCheckComponent::CheckVaultLandingFloor(const FHitResult& FrontHit, const FHitResult& TopHit, const FHitResult& BackHit, FHitResult& OutHit) const
//{
//	ACharacter* Owner = Cast<ACharacter>(GetOwner());
//
//	if (false == IsValid(Owner))
//	{
//		return false;
//	}
//
//	if (false == FrontHit.bBlockingHit || false == TopHit.bBlockingHit || false == BackHit.bBlockingHit)
//	{
//		return false;
//	}
//
//	if (nullptr == GetWorld())
//	{
//		return false;
//	}
//
//	const FVector UpVector = Owner->GetActorUpVector();
//
//	// 앞면에서 장애물 뒤쪽으로 수평 방향
//	const FVector IntoBlock = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();
//
//	if (IntoBlock.IsNearlyZero())
//	{
//		return false;
//	}
//
//
//	// BackHit 를 장애물 윗면 높이까지 올린 위치
//	const float BackToTopHeight = FVector::DotProduct(TopHit.ImpactPoint - BackHit.ImpactPoint, UpVector);
//
//	const FVector BackTopPoint = BackHit.ImpactPoint + UpVector * BackToTopHeight;
//
//	// 뒷면에서 캐릭터 캡슐 콜라이더보다 조금 앞 위치
//	const FVector CheckLocation = BackTopPoint + IntoBlock * LandingCheckFront;
//
//	const FVector Start = CheckLocation + UpVector * LandingCheckUp;
//
//	const FVector End = CheckLocation - UpVector * LandingCheckDown;
//
//	FCollisionQueryParams QueryParams;
//
//	QueryParams.AddIgnoredActor(Owner);
//
//	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel.GetValue(), QueryParams);
//
//	if (bDrawDebug)
//	{
//		const FVector DebugEnd = bCollision ? OutHit.ImpactPoint : End;
//
//		const FColor DebugColor = bCollision ? FColor::Yellow : FColor::Red;
//
//		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);
//
//		if (bCollision)
//		{
//			DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 8.0f, 12, FColor::Orange, false, 2.0f);
//		}
//	}
//
//	return bCollision;
//}

//bool UHurdleCheckComponent::CheckMantleTopFloor(const FHitResult& FrontHit, const FHitResult& TopHit, FHitResult& OutHit) const
//{
//	OutHit = FHitResult();
//
//	ACharacter* Owner = Cast<ACharacter>(GetOwner());
//
//	if (false == IsValid(Owner))
//	{
//		return false;
//	}
//
//	const UCapsuleComponent* CharacterCapsule = Owner->GetCapsuleComponent();
//
//	if (false == IsValid(CharacterCapsule))
//	{
//		return false;
//	}
//
//	if (false == FrontHit.bBlockingHit || false == TopHit.bBlockingHit)
//	{
//		return false;
//	}
//
//	if (nullptr == GetWorld())
//	{
//		return false;
//	}
//
//	const FVector UpVector = Owner->GetActorUpVector();
//
//	const FVector IntoObstacle = FVector::VectorPlaneProject(-FrontHit.ImpactNormal, UpVector).GetSafeNormal();
//
//	if (IntoObstacle.IsNearlyZero())
//	{
//		return false;
//	}
//
//	// 앞면 충돌점을 윗면 높이까지 올려서 모서리 위치를 구하기
//	const float TopHeightFromFront = FVector::DotProduct(TopHit.ImpactPoint - FrontHit.ImpactPoint, UpVector);
//
//	const FVector FrontPointAtTopHeight = FrontHit.ImpactPoint + UpVector * TopHeightFromFront;
//
//	const float CapsuleRadius = CharacterCapsule->GetScaledCapsuleRadius();
//
//	// 캡슐이 올라설 수 있게 모서리 안쪽으로 옮겨서 확인하기
//	const FVector CheckLocation = FrontPointAtTopHeight + IntoObstacle * (CapsuleRadius + TopCheckInset);
//
//	const FVector Start = CheckLocation + UpVector * TopFloorCheckDistance;
//
//	const FVector End = CheckLocation - UpVector * TopFloorCheckDistance;
//
//	FCollisionQueryParams QueryParams;
//
//	QueryParams.AddIgnoredActor(Owner);
//
//	const bool bCollision = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel.GetValue(), QueryParams);
//
//	if (bDrawDebug)
//	{
//		const FVector DebugEnd = bCollision ? OutHit.ImpactPoint : End;
//
//		const FColor DebugColor = bCollision ? FColor::Purple : FColor::Red;
//
//		DrawDebugLine(GetWorld(), Start, DebugEnd, DebugColor, false, 2.0f, 0, 2.0f);
//
//		if (bCollision)
//		{
//			DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 8.0f, 12, FColor::Purple, false, 2.0f);
//		}
//	}
//
//	return bCollision;
//}

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

	//// 캐릭터 캡슐과 같은 충돌 프로필로 공간을 검사하고 막힌게 있다면 착지 공간을 사용할 수 없고 막힌게 없으면 착지할 공간이 사용가능
	//const bool bBlocked = GetWorld()->OverlapBlockingTestByProfile(CapsuleCenter, FQuat::Identity, CharacterCapsule->GetCollisionProfileName(), CapsuleShape, QueryParams);

	//if (bDrawDebug)
	//{
	//	const FColor DebugColor = bBlocked ? FColor::Red : FColor::Green;

	//	DrawDebugCapsule(GetWorld(), CapsuleCenter, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, DebugColor, false, 2.0f);
	//}

	return false == Blocked;

}