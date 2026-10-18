// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/GrenadeActor.h"
#include "MolotovActor.generated.h"

class AFireAreaActor;
class UPrimitiveComponent;

// 화염병. 쿠킹 없이 던지고, 던진 뒤 어딘가에 닿으면 바로 깨지면서 바닥에 불 장판을 만든다.
UCLASS()
class PROJECTFPS_API AMolotovActor : public AGrenadeActor
{
	GENERATED_BODY()

protected:
	// 깨진 자리에 생성할 불 장판. 반경·지속시간·데미지는 불 장판 BP가 가진다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Molotov")
	TSubclassOf<AFireAreaActor> _FireAreaClass;

	// 깨진 위치에서 아래로 바닥을 찾는 거리. 벽·천장에 맞아도 이 거리 안의 바닥에 불이 깔린다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Molotov", meta = (ClampMin = "0.0", Units = "cm"))
	float _FloorTraceDistance = 300.f;

private:
	// 한 번 깨지면 다시 맞아도 무시한다
	bool _bShattered = false;

public:
	AMolotovActor();

protected:
	virtual void BeginPlay() override;

	// 불 장판 생성
	virtual void Detonate(const FVector& ExplosionLocation) override;

private:
	// 던져진 뒤 무언가에 닿으면 깨진다. (서버에서만 연결)
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
