// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/BombGrenadeActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "GameMode/PlayerStateBase.h"
#include "GameTag/FPSGameplayTag.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

void ABombGrenadeActor::Detonate(const FVector& ExplosionLocation)
{
	// 데미지 설정이 없으면 터지기만 한다. (다음 장착과 삭제는 LifeSpanExpired가 계속 처리)
	if (_DamageEffect == nullptr || !FMath::IsFinite(_ExplosionRadius) || _ExplosionRadius <= 0.f || !FMath::IsFinite(_Damage) || _Damage <= 0.f)
	{
		return;
	}

	// 수류탄의 폭발범위에 걸친 Pawn을 검색 -> 이때 수류탄을 던진 플레이어도 포함
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 이코드는 잘 이해못함 무슨 문법인지 잘 모르겠어요 그래서 노션에 정리해 두었습니다. - 건영

	TArray<AActor*> ActorsToIgnore;

	ActorsToIgnore.Add(this);

	TArray<AActor*> Targets;

	UKismetSystemLibrary::SphereOverlapActors(this, ExplosionLocation, _ExplosionRadius, ObjectTypes, APawn::StaticClass(), ActorsToIgnore, Targets);

	const UDamageSourceComponent* Source = FindComponentByClass<UDamageSourceComponent>();

	APlayerStateBase* FPSPlayerState = IsValid(Source) ? Source->GetSourcePlayerState() : nullptr;

	// 플레이어끼리도 서로 폭발을 막는 벽으로 취급하지 않기
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GrenadeExplosion), false, this);

	TraceParams.AddIgnoredActors(Targets);

	for (AActor* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

		if (!IsValid(TargetASC))
		{
			continue;
		}

		// 폭발 지점부터 대상 중심까지 벽이 막고 있으면 피해를 주지 못함
		// 중심과 상체 위쪽이 모두 가려졌을 때만 폭발 피해를 차단한다
		if (GetWorld()->LineTraceTestByChannel(ExplosionLocation, Target->GetActorLocation(), ECC_Visibility, TraceParams) &&
			GetWorld()->LineTraceTestByChannel(ExplosionLocation, Target->GetActorLocation() + FVector::UpVector * Target->GetSimpleCollisionHalfHeight() * 0.75f, ECC_Visibility, TraceParams))
		{
			continue;
		}

		FGameplayEffectContextHandle Context(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());

		Context.AddInstigator(FPSPlayerState, this);
		Context.AddSourceObject(this);

		FGameplayEffectSpec Spec(_DamageEffect.GetDefaultObject(), Context, 1.f);

		Spec.SetSetByCallerMagnitude(FPSGameplayTags::SetByCaller_Damage, _Damage);

		TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
}
