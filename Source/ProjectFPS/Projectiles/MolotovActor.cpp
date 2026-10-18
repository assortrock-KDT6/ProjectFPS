// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/MolotovActor.h"
#include "Projectiles/FireAreaActor.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

AMolotovActor::AMolotovActor()
{
	// 핀을 뽑는 무기가 아니라서 쿠킹하지 않는다.
	_bCanCook = false;

	// 아무 데도 닿지 않고 굴러다닐 때를 대비한 최대 시간. 보통은 닿는 순간 깨진다.
	_FuseTime = 10.f;
}

void AMolotovActor::BeginPlay()
{
	Super::BeginPlay();

	if (true == HasAuthority() && nullptr != _GrenadeCollision)
	{
		// 물리 시뮬레이션 중 충돌을 Hit 이벤트로 받는다. (손에 든 동안은 충돌이 꺼져 있어서 호출되지 않음)
		_GrenadeCollision->SetNotifyRigidBodyCollision(true);
		_GrenadeCollision->OnComponentHit.AddUniqueDynamic(this, &AMolotovActor::HandleHit);
	}
}

void AMolotovActor::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (false == HasAuthority() || false == IsThrown() || true == _bShattered)
	{
		return;
	}

	// 던진 사람과 그 사람이 가진 것(들고 있는 무기 등)에 닿은 건 무시 (던진 직후 손에 걸리는 경우)
	AActor* Thrower = GetOwner();
	if (nullptr != OtherActor && nullptr != Thrower && (OtherActor == Thrower || OtherActor->GetOwner() == Thrower))
	{
		return;
	}

	_bShattered = true;

	// 물리 콜백 안에서 바로 지우지 않고, 다음 틱에 기존 폭발 경로(LifeSpanExpired)로 터뜨린다.
	SetLifeSpan(0.01f);
}

void AMolotovActor::Detonate(const FVector& ExplosionLocation)
{
	UWorld* World = GetWorld();
	if (nullptr == World || nullptr == _FireAreaClass)
	{
		return;
	}

	// 깨진 위치 아래의 바닥을 찾는다. 바닥이 없으면(공중) 깨진 위치에 깐다.
	FVector FireLocation = ExplosionLocation;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MolotovFloorTrace), false, this);
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult FloorHit;
	const FVector TraceEnd = ExplosionLocation - FVector::UpVector * _FloorTraceDistance;
	if (true == World->LineTraceSingleByChannel(FloorHit, ExplosionLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		FireLocation = FloorHit.ImpactPoint;
	}

	// 처치 기록은 화염병을 던진 사람에게 가야 하므로 PlayerState를 불 장판에 넘긴다.
	APlayerStateBase* SourcePlayerState = nullptr;
	const UDamageSourceComponent* Source = FindComponentByClass<UDamageSourceComponent>();
	if (nullptr != Source)
	{
		SourcePlayerState = Source->GetSourcePlayerState();
	}

	AFireAreaActor* FireArea = World->SpawnActorDeferred<AFireAreaActor>(_FireAreaClass, FTransform(FireLocation), GetOwner(), GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (nullptr == FireArea)
	{
		return;
	}

	FireArea->SetSourcePlayerState(SourcePlayerState);
	FireArea->FinishSpawning(FTransform(FireLocation));
}
