// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/FlashbangActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AFlashbangActor::AFlashbangActor()
{
	// 섬광탄은 굴러간 뒤 바로 터지도록 신관을 짧게 둔다.
	_FuseTime = 1.5f;
}

void AFlashbangActor::Detonate(const FVector& ExplosionLocation)
{
	// 폭발 이펙트·소리는 LifeSpanExpired가 이미 보낸다. 여기서는 섬광만 알린다.
	MulticastFlash(ExplosionLocation);
}

void AFlashbangActor::MulticastFlash_Implementation(const FVector& FlashLocation)
{
	// 화면이 없는 전용 서버에서는 계산하지 않는다.
	if (NM_DedicatedServer == GetNetMode())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (nullptr == World)
	{
		return;
	}

	// 이 화면의 플레이어 눈 = 카메라
	APlayerController* LocalController = World->GetFirstPlayerController();
	if (nullptr == LocalController || false == LocalController->IsLocalController() || nullptr == LocalController->PlayerCameraManager)
	{
		return;
	}

	APlayerCameraManager* CameraManager = LocalController->PlayerCameraManager;
	const FVector EyeLocation = CameraManager->GetCameraLocation();
	const FVector ToFlash = FlashLocation - EyeLocation;
	const float Distance = ToFlash.Size();

	// 너무 멀면 영향 없음
	if (Distance > _FlashRadius)
	{
		return;
	}

	// 섬광과 내 눈 사이가 벽으로 막혀 있으면 영향 없음 (던진 본인도 포함해서 계산)
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlashbangTrace), false, this);
	QueryParams.AddIgnoredActor(LocalController->GetPawn());
	if (true == World->LineTraceTestByChannel(EyeLocation, FlashLocation, ECC_Visibility, QueryParams))
	{
		return;
	}

	// 바라보는 정도: 정면 1, 정반대로 등지면 _BehindStrength
	const float Facing = FVector::DotProduct(CameraManager->GetCameraRotation().Vector(), ToFlash.GetSafeNormal());
	const float FacingStrength = FMath::Lerp(_BehindStrength, 1.f, (Facing + 1.f) * 0.5f);

	// 거리 감쇠: _FullBlindRadius 안은 1, 그 밖은 반경 끝으로 갈수록 0
	float DistanceStrength = 1.f;
	if (Distance > _FullBlindRadius && _FlashRadius > _FullBlindRadius)
	{
		DistanceStrength = 1.f - ((Distance - _FullBlindRadius) / (_FlashRadius - _FullBlindRadius));
	}

	const float Strength = FMath::Clamp(FacingStrength * DistanceStrength, 0.f, 1.f);
	if (Strength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float HoldTime = _MaxHoldDuration * Strength;
	const float FadeTime = _MaxFlashDuration * Strength;

	// 유지 시간이 없으면 바로 원래 화면으로 돌아온다.
	if (HoldTime <= KINDA_SMALL_NUMBER)
	{
		CameraManager->StartCameraFade(Strength, 0.f, FadeTime, FLinearColor::White, false, false);
	}
	else
	{
		// 하얀 화면을 유지하다가(bHoldWhenFinished) 유지 시간이 끝나면 원래 화면으로 돌아온다.
		// 섬광탄 액터는 터진 직후 사라지므로 타이머는 카메라 매니저 기준으로 건다.
		CameraManager->StartCameraFade(Strength, Strength, HoldTime, FLinearColor::White, false, true);

		FTimerHandle FadeOutHandle;
		World->GetTimerManager().SetTimer(FadeOutHandle, FTimerDelegate::CreateWeakLambda(CameraManager, [CameraManager, Strength, FadeTime]()
		{
			CameraManager->StartCameraFade(Strength, 0.f, FadeTime, FLinearColor::White, false, false);
		}), HoldTime, false);
	}

	if (nullptr != _RingingSound)
	{
		UGameplayStatics::PlaySound2D(this, _RingingSound, Strength);
	}
}
