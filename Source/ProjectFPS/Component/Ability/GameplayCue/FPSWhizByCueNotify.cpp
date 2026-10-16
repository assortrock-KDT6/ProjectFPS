// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Ability/GameplayCue/FPSWhizByCueNotify.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"


bool UFPSWhizByCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (nullptr == _WhizSound || nullptr == MyTarget)
	{
		return false;
	}

	UWorld* World = MyTarget->GetWorld();
	if (nullptr == World || NM_DedicatedServer == World->GetNetMode())
	{
		return false;
	}

	// 이 화면의 플레이어. 귀 위치 = 카메라 위치
	APlayerController* LocalController = World->GetFirstPlayerController();
	if (nullptr == LocalController || false == LocalController->IsLocalController() || nullptr == LocalController->PlayerCameraManager)
	{
		return false;
	}

	// 내가 쏜 총알은 내 귓가를 스치지 않는다.
	APawn* LocalPawn = LocalController->GetPawn();
	AActor* Shooter = Parameters.Instigator.Get();
	if (nullptr == LocalPawn || LocalPawn == Shooter)
	{
		return false;
	}

	const FVector Ear = LocalController->PlayerCameraManager->GetCameraLocation();
	const FVector Start = Parameters.Location;
	const FVector Direction = FVector(Parameters.Normal).GetSafeNormal();
	if (true == Direction.IsNearlyZero())
	{
		return false;
	}

	FVector End = Start + Direction * Parameters.RawMagnitude;

	// 벽 검사: 총알이 멈추는 곳까지만 본다. 나에게 맞는 총알이면 스치는 소리가 아니라 피격 연출이 난다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WhizByTrace), false);
	QueryParams.AddIgnoredActor(Shooter);

	FHitResult Hit;
	if (true == World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		if (Hit.GetActor() == LocalPawn)
		{
			return false;
		}
		End = Hit.ImpactPoint;
	}

	// 궤적에서 귀와 가장 가까운 점
	const FVector PassPoint = FMath::ClosestPointOnSegment(Ear, Start, End);
	if (FVector::Dist(PassPoint, Ear) > _MaxDistance)
	{
		return false;
	}

	// 총알이 그 점에 도착하는 순간에 재생한다.
	USoundBase* Sound = _WhizSound;
	const float Delay = FVector::Dist(Start, PassPoint) / _BulletSpeed;
	if (Delay <= KINDA_SMALL_NUMBER)
	{
		UGameplayStatics::PlaySoundAtLocation(World, Sound, PassPoint);
		return true;
	}

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(World, [World, Sound, PassPoint]()
		{
			UGameplayStatics::PlaySoundAtLocation(World, Sound, PassPoint);
		}), Delay, false);

	return true;
}
