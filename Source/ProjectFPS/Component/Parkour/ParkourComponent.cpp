// Fill out your copyright notice in the Description page of Project Settings.

#include "ParkourComponent.h"
#include "Component/Parkour/ParkourComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UParkourComponent::TryParkour()
{
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

	if (MaxVaultDepth <= 0.0f)
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
	const FVector CheckLocation = FrontPointAtTopHeight - UpVector * BackCheckHeightOffset;
	
	// 장애물 너머에서 시작하여 앞면 방향으로 역방향 Line Trace 를 쏜다.
	const FVector Start = CheckLocation + IntoBlock * MaxVaultDepth;

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
