// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/GrenadeActor.h"
#include "SmokeGrenadeActor.generated.h"

class ASmokeAreaActor;

/**
 *  연막탄
 */
UCLASS()
class PROJECTFPS_API ASmokeGrenadeActor : public AGrenadeActor
{
	GENERATED_BODY()

protected:
	// 연막의 지속시간과 이펙트는 해당 클래스에서 설정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Smoke")
	TSubclassOf<ASmokeAreaActor> _SmokeAreaClass;

	// 첫 수명 만료는 연막 발동, 다음 수명 만료는 제거로 구분한다.
	// 발동 판단은 서버에서만 하므로 복제할 필요가 없다.
	bool _bActivated = false;

	virtual void LifeSpanExpired() override;

	virtual void Detonate(const FVector& ExplosionLocation) override;
};
