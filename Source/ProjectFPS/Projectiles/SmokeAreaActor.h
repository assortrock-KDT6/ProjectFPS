// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmokeAreaActor.generated.h"

class UParticleSystemComponent;
class UNiagaraComponent;

/**
 * 연막탄이 발동한 위치에 생성되는 연막.
 * 서버가 지속시간을 관리하고, 각 화면에서 Niagara를 재생한다.
 */

UCLASS()
class PROJECTFPS_API ASmokeAreaActor : public AActor
{
	GENERATED_BODY()

protected:
	// 연막 이펙트. 사용할 파티클과 크기는 BP에서 설정한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smoke | Effect")
	TObjectPtr<UParticleSystemComponent> _SmokeEffect = nullptr;

	// 연막이 유지되는 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smoke", meta = (ClampMin = "0.1", Units = "s"))
	float _Duration = 15.f;

public:
	ASmokeAreaActor();

protected:
	virtual void BeginPlay() override;
};