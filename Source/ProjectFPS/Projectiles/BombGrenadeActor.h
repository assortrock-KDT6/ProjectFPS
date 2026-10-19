// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/GrenadeActor.h"
#include "BombGrenadeActor.generated.h"

class UGameplayEffect;

// 기존 폭탄(파편 수류탄). 터지면 범위 안의 폰에게 데미지를 준다.
UCLASS()
class PROJECTFPS_API ABombGrenadeActor : public AGrenadeActor
{
	GENERATED_BODY()

protected:
	// 폭발 피해를 검색할 반경, 단위는 cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Explosion", meta = (ClampMin = "1.0", Units = "cm"))
	float _ExplosionRadius = 500.f;

	// 기존 피해 GE에 전달할 수류탄의 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Explosion", meta = (ClampMin = "0.0"))
	float _Damage = 150.f;

	// 범위 안 대상에게 적용할 피해 GE. _Damage 값을 SetByCaller로 넘긴다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Explosion")
	TSubclassOf<UGameplayEffect> _DamageEffect;

protected:
	// 범위 데미지
	virtual void Detonate(const FVector& ExplosionLocation) override;
};
