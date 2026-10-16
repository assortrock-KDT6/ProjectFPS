// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "FPSWhizByCueNotify.generated.h"

/**
 * 총알이 귓가를 스치는 소리(Near miss)
 * 서버가 발사 위치 방향 사거리를 보내면, 각 화면이 그 궤적이 내 귀근처를 지나는지 계산해서 재생한다.
 */
class USoundBase;
UCLASS()
class PROJECTFPS_API UFPSWhizByCueNotify : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

protected:
	// 스치는 소리
	UPROPERTY(EditDefaultsOnly, Category = "WhizBy")
	TObjectPtr<USoundBase> _WhizSound;
	
	// 궤적이 귀에서 들리는 범위
	UPROPERTY(EditDefaultsOnly, Category = "WhizBy", meta = (ClampMin = "10.0", Units = "cm"))
	float _MaxDistance = 300.f;

	// 총알 속도. 소리를 총알이 도작하는 순간에 맞추는데 사용함. 
	UPROPERTY(EditDefaultsOnly, Category = "WhizBy", meta = (ClampMin = "1.0", Units = "cm/s"))
	float _BulletSpeed = 10000.f;
public: 
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
