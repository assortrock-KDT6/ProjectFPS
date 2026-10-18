// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/GrenadeActor.h"
#include "FlashbangActor.generated.h"

class USoundBase;

// 섬광탄. 데미지는 없고, 터지면 각 화면이 자기 시야·거리를 계산해서 화면을 하얗게 만든다.
UCLASS()
class PROJECTFPS_API AFlashbangActor : public AGrenadeActor
{
	GENERATED_BODY()

protected:
	// 이 거리 안에 있어야 섬광의 영향을 받는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Flashbang", meta = (ClampMin = "1.0", Units = "cm"))
	float _FlashRadius = 1500.f;

	// 이 거리 안에서는 거리 감쇠 없이 최대 강도로 맞는다 (밖에서는 _FlashRadius까지 줄어든다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Flashbang", meta = (ClampMin = "0.0", Units = "cm"))
	float _FullBlindRadius = 600.f;

	// 최대 강도일 때 완전히 하얀 화면이 유지되는 시간 (강도에 비례해서 줄어든다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Flashbang", meta = (ClampMin = "0.0", Units = "s"))
	float _MaxHoldDuration = 1.5f;

	// 최대 강도일 때 하얀 화면이 유지된 뒤 원래 화면으로 돌아오기까지 걸리는 시간 (강도에 비례)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Flashbang", meta = (ClampMin = "0.1", Units = "s"))
	float _MaxFlashDuration = 4.f;

	// 섬광을 등지고 있을 때 받는 강도 (정면 = 1). 0이면 등지면 효과 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Flashbang", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float _BehindStrength = 0.2f;

	// 귀 울림. 내 귀에서 들리는 소리라 2D로 재생한다.
	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Flashbang")
	TObjectPtr<USoundBase> _RingingSound = nullptr;

public:
	AFlashbangActor();

protected:
	// 범위 데미지 대신 모든 화면에 섬광 위치를 알린다.
	virtual void Detonate(const FVector& ExplosionLocation) override;

private:
	// 각 화면이 자기 카메라 기준으로 섬광 강도를 계산해서 적용한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFlash(const FVector& FlashLocation);
};
