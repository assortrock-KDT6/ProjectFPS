// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/SmokeAreaActor.h"
#include "Particles/ParticleSystemComponent.h"

ASmokeAreaActor::ASmokeAreaActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	// 위치는 부착된 연막탄을 따라가므로 별도의 이동 복제는 하지 않는다.
	SetReplicateMovement(false);

	_SmokeEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SmokeParticle"));

	SetRootComponent(_SmokeEffect);

	// 재생은 BeginPlay에서 시작하고, 제거는 액터 수명에 맡긴다.
	_SmokeEffect->SetAutoActivate(false);
	_SmokeEffect->bAutoDestroy = false;
}

// Called when the game starts or when spawned
void ASmokeAreaActor::BeginPlay()
{
	Super::BeginPlay();

	// 위치만 본체를 따라간다.
	// 연막탄이 회전하거나 굴러도 연기의 방향과 크기는 유지한다.
	_SmokeEffect->SetAbsolute(false, true, true);
	_SmokeEffect->SetWorldRotation(FRotator::ZeroRotator);

	if (true == HasAuthority())
	{
		SetLifeSpan(FMath::Max(_Duration, 0.1f));
	}

	if (NM_DedicatedServer == GetNetMode())
	{
		return;
	}

	_SmokeEffect->Activate(true);
}
