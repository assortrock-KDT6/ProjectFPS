// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/FireAreaActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "GameMode/PlayerStateBase.h"
#include "GameTag/FPSGameplayTag.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFireAreaActor::AFireAreaActor()
{
	// 넓어지는 동안 이펙트 반경을 갱신할 때만 Tick 사용 (다 넓어지면 끈다)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 서버가 생성하면 모든 화면에 나타나고, 서버에서 사라지면 같이 사라진다.
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	_DamageSource = CreateDefaultSubobject<UDamageSourceComponent>(TEXT("DamageSource"));
}

void AFireAreaActor::SetSourcePlayerState(APlayerStateBase* PlayerState)
{
	_DamageSource->SetSourcePlayerState(PlayerState);
}

void AFireAreaActor::InitializeDamageSource(APlayerStateBase* PlayerState, const FDamageWeaponInfo& DamageWeapon)
{
	SetSourcePlayerState(PlayerState);
	_DamageSource->InitializeDamageWeapon(DamageWeapon);
}

void AFireAreaActor::BeginPlay()
{
	Super::BeginPlay();

	_SpawnTime = GetWorld()->GetTimeSeconds();

	if (true == HasAuthority())
	{
		SetLifeSpan(_Duration);

		// 깔리는 순간 한 번, 그 뒤로 간격마다 데미지
		ApplyFireDamage();
		GetWorldTimerManager().SetTimer(_DamageTimerHandle, this, &AFireAreaActor::ApplyFireDamage, _TickInterval, true);
	}

	// 화면이 없는 전용 서버에서는 연출하지 않는다.
	if (NM_DedicatedServer == GetNetMode())
	{
		return;
	}

	// 장판에 붙여서 재생하면 장판이 사라질 때 같이 사라진다.
	if (nullptr != _FireEffect)
	{
		_FireEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(_FireEffect, GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, _FireEffectScale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, true);

		// 처음 반경을 넘겨주고, 넓어지는 동안 Tick으로 갱신
		UpdateEffectRadius();
		if (nullptr != _FireEffectComponent && _SpreadTime > 0.f)
		{
			SetActorTickEnabled(true);
		}
	}

	if (nullptr != _FireLoopSound)
	{
		// 기본값은 붙은 액터가 사라져도 소리가 끝까지 재생된다. 불이 꺼질 때 같이 멈추도록 bStopWhenAttachedToDestroyed = true
		UGameplayStatics::SpawnSoundAttached(_FireLoopSound, GetRootComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true);
	}
}

void AFireAreaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(_DamageTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AFireAreaActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateEffectRadius();

	// 다 넓어지면 더 갱신할 필요가 없다.
	if (GetWorld()->GetTimeSeconds() - _SpawnTime >= _SpreadTime)
	{
		SetActorTickEnabled(false);
	}
}

float AFireAreaActor::GetCurrentRadius() const
{
	if (_SpreadTime <= 0.f)
	{
		return _Radius;
	}

	const float Alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - _SpawnTime) / _SpreadTime, 0.f, 1.f);

	return FMath::Lerp(FMath::Min(_StartRadius, _Radius), _Radius, Alpha);
}

void AFireAreaActor::UpdateEffectRadius()
{
	if (nullptr == _FireEffectComponent || _EffectRadiusParameter.IsNone())
	{
		return;
	}

	// 이펙트 크기 배율이 반경에도 곱해지므로 나눠서 넘긴다. (보이는 불 = 데미지 범위)
	float EffectScale = _FireEffectScale.X;
	if (EffectScale <= KINDA_SMALL_NUMBER)
	{
		EffectScale = 1.f;
	}

	_FireEffectComponent->SetVariableFloat(_EffectRadiusParameter, GetCurrentRadius() / EffectScale);
}

void AFireAreaActor::ApplyFireDamage()
{
	UWorld* World = GetWorld();
	if (nullptr == World || nullptr == _DamageEffect || _DamagePerTick <= 0.f)
	{
		return;
	}

	const FVector FireLocation = GetActorLocation();

	// 장판 범위 안의 폰 검색 (던진 사람도 포함)
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<AActor*> Targets;
	// 지금까지 넓어진 반경만큼만 탄다.
	UKismetSystemLibrary::SphereOverlapActors(this, FireLocation, GetCurrentRadius(), ObjectTypes, APawn::StaticClass(), ActorsToIgnore, Targets);

	// 벽 너머(다른 층 포함)에 있는 대상은 타지 않는다. 플레이어끼리는 서로 가리지 않음
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(FireAreaTrace), false, this);
	TraceParams.AddIgnoredActors(Targets);

	// 불 위치는 바닥이라 살짝 띄워서 검사한다. (바닥 자체에 막히지 않게)
	const FVector TraceStart = FireLocation + FVector::UpVector * 30.f;

	for (AActor* Target : Targets)
	{
		if (false == IsValid(Target))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		if (nullptr == TargetASC)
		{
			continue;
		}

		if (true == World->LineTraceTestByChannel(TraceStart, Target->GetActorLocation(), ECC_Visibility, TraceParams))
		{
			continue;
		}

		FGameplayEffectContextHandle Context(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());
		Context.AddInstigator(_DamageSource->GetSourcePlayerState(), this);
		Context.AddSourceObject(this);

		FGameplayEffectSpec Spec(_DamageEffect.GetDefaultObject(), Context, 1.f);
		Spec.SetSetByCallerMagnitude(FPSGameplayTags::SetByCaller_Damage, _DamagePerTick);

		TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
}
