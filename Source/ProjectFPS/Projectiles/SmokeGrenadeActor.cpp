// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/SmokeGrenadeActor.h"
#include "Projectiles/SmokeAreaActor.h"
#include "Engine/World.h"

void ASmokeGrenadeActor::LifeSpanExpired()
{
	if (true == _bActivated)
	{
		// 연막 지속시간이 끝났으므로 다시 발동하지 않고 제거한다.
		AActor::LifeSpanExpired();
		return;
	}

	// 첫 만료에서는 부모의 장착 정리와 Detonate 호출을 사용한다.
	Super::LifeSpanExpired();
}

void ASmokeGrenadeActor::Detonate(const FVector& ExplosionLocation)
{
	UWorld* World = GetWorld();

	if (false == HasAuthority() || true == _bActivated || nullptr == World || nullptr == _SmokeAreaClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASmokeAreaActor* SmokeArea = World->SpawnActor<ASmokeAreaActor>(_SmokeAreaClass, FTransform(ExplosionLocation), SpawnParameters);

	if (false == IsValid(SmokeArea))
	{
		return;
	}

	// 연기가 새로 나오는 위치는 연막탄 본체를 따라간다.
	if (SmokeArea->GetLifeSpan() <= 0.f || false == SmokeArea->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform))
	{
		SmokeArea->Destroy();
		return;
	}

	// 신관 수명을 연막 지속시간으로 전환한다.
	// Drop은 남은 수명이 있어야 실행되므로 먼저 설정한다.
	SetLifeSpan(SmokeArea->GetLifeSpan());

	// 손에서 발동했다면 떨어뜨린다.
	// 이미 던진 경우에는 기존 비행 속도를 그대로 유지한다.
	if (false == IsThrown() && false == Drop())
	{
		SmokeArea->Destroy();
		SetLifeSpan(0.f);
		return;
	}

	_bActivated = true;

	SmokeArea->ForceNetUpdate();
	ForceNetUpdate();
}