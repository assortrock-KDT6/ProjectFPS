// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "UI/GamePlay/PlayerStatusWidget.h"
#include "UI/GameHUD.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectAggregator.h"
#include "Character/CharacterPlayer.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "GameMode/FPSGameMode.h"
#include "GameMode/PlayerStateBase.h"
#include "GameTag/FPSGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"

UFPSHealthSet::UFPSHealthSet()
{
	// TODO : Character State Setting
	Init_MaxHealth(100.f);
	Init_Health(_MaxHealth.GetBaseValue());
	Init_MaxShield(100.f);
	Init_Shield(_MaxShield.GetBaseValue());
	Init_DamageIn(0.f);
}

void UFPSHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UFPSHealthSet, _Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSHealthSet, _MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSHealthSet, _Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSHealthSet, _MaxShield, COND_None, REPNOTIFY_Always);

}

void UFPSHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Get_HealthAttribute() == Attribute)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, Get_MaxHealth());
	}
	else if (Get_ShieldAttribute() == Attribute)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, Get_MaxShield());
	}
}

bool UFPSHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	// 피해는 서버의 경기 시계로 판정한다. 만료 타이머가 실행되기 전 프레임도 차단한다.
	if (Data.EvaluatedData.Attribute == Get_DamageInAttribute()
		|| Data.EvaluatedData.Attribute == Get_HealthAttribute()
		|| Data.EvaluatedData.Attribute == Get_ShieldAttribute())
	{
		const AActor* Avatar = Data.Target.GetAvatarActor();

		const AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();

		if (false == IsValid(Avatar) || false == Avatar->HasAuthority() || false == IsValid(GameMode))
		{
			return false;
		}

		const bool DamageBlocked = (false == GameMode->IsCombatAllowed()) || Data.Target.HasMatchingGameplayTag(FPSGameplayTags::Status_Damage_Immune);

		if (Data.EvaluatedData.Attribute == Get_DamageInAttribute()
			&& (DamageBlocked || Get_Health() <= 0.f))
		{
			return false;
		}

		// 경기 전 초기화 GE는 허용하되 종료 시각 이후의 속성 변경은 막는다.

		if (GameMode->HasMatchStarted() && !GameMode->IsCombatAllowed())
		{
			return false;
		}

		if (DamageBlocked && Data.EvaluatedData.Attribute != Get_DamageInAttribute())
		{
			// 초기화/회복은 허용하지만 직접 체력/실드를 깎는 GE는 면역 중에도 차단한다.
			const float CurrentValue = Data.EvaluatedData.Attribute == Get_HealthAttribute() ? Get_Health() : Get_Shield();
			const float NewValue = FAggregator::StaticExecModOnBaseValue(CurrentValue, Data.EvaluatedData.ModifierOp, Data.EvaluatedData.Magnitude);
			if (NewValue < CurrentValue)
			{
				return false;
			}
		}
	}
	return Super::PreGameplayEffectExecute(Data);
}

void UFPSHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Get_DamageInAttribute() == Data.EvaluatedData.Attribute)
	{
		const float LocalDamageIn = Get_DamageIn();
		Set_DamageIn(0.f);

		if (LocalDamageIn > 0.f)
		{
			float CurrentShield = Get_Shield();
			float CurrentHealth = Get_Health();
			float DamageToApply = LocalDamageIn;
			float ShieldDamage = 0.f;	// 쉴드가 흡수한 양.

			if (CurrentShield > 0.f)
			{
				if (CurrentShield >= DamageToApply)
				{
					Set_Shield(CurrentShield - DamageToApply);
					ShieldDamage = DamageToApply; //추가
					DamageToApply = 0.f;
				}
				else 
				{
					ShieldDamage = CurrentShield; // 추가 
					DamageToApply -= CurrentShield;
					Set_Shield(0.f);
				}
			}

			// 추가 : 쉴드가 줄어들면 쉴드 피격 연출
			if (ShieldDamage > 0.f)
			{
				ExecuteHitCue(Data, FPSGameplayTags::GameplayCue_Character_Hit_Shield, ShieldDamage);
			}

			if (DamageToApply > 0.f)
			{
				const float NewHealth = FMath::Clamp(CurrentHealth - DamageToApply, 0.f, Get_MaxHealth());
				Set_Health(NewHealth);

				// 추가: 체력이 깎였으면 피 연출
				if (NewHealth < CurrentHealth)
				{
					ExecuteHitCue(Data, FPSGameplayTags::GameplayCue_Character_Hit_Health, CurrentHealth - NewHealth);
				}

				if (NewHealth <= 0.f && CurrentHealth > 0.f)
				{
					_OnOutOfHealth.Broadcast(GetDamagePlayerState(Data));
				}

			}
		}
	}
}

// BP가 EffectCauser만 지정한 경우에도 발사 때 저장한 PlayerState를 우선 사용한다.
APlayerStateBase* UFPSHealthSet::GetDamagePlayerState(const FGameplayEffectModCallbackData& Data) const
{
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetContext();

	APlayerStateBase* PlayerState = FindDamagePlayerState(Context.GetEffectCauser());

	if (!IsValid(PlayerState))
	{
		PlayerState = FindDamagePlayerState(Context.GetOriginalInstigator());
	}

	return PlayerState;
}

void UFPSHealthSet::ExecuteHitCue(const FGameplayEffectModCallbackData& Data, const FGameplayTag& CueTag, float Damage) const
{
	UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent();
	AActor* Avatar = Data.Target.GetAvatarActor();

	if (false == IsValid(AbilitySystem) || false == IsValid(Avatar))
	{
		return;
	}

	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetContext();
	AActor* Causer = Context.GetEffectCauser();

	// 가해 액터를 모르면 피격자 중심에서 재생한다.
	FVector HitLocation = Avatar->GetActorLocation();
	FVector HitNormal = FVector::UpVector;

	if (true == IsValid(Causer))
	{
		const FVector CauserLocation = Causer->GetActorLocation();
		const FVector CauserVelocity = Causer->GetVelocity();
		UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(Avatar->GetRootComponent());

		bool bFoundEntry = false;

		// 총알은 빨라서 판정 시점에 이미 몸을 지나쳐 있을 수 있다.
		// 날아온 경로를 거슬러 올라가 몸에 처음 들어간 지점을 찾는다.
		if (false == CauserVelocity.IsNearlyZero() && nullptr != Collision)
		{
			const FVector TravelDirection = CauserVelocity.GetSafeNormal();

			// 한 프레임 이동 거리 + 캡슐 두께 여유만큼 뒤에서 시작
			const float TraceBackDistance = CauserVelocity.Size() * Avatar->GetWorld()->GetDeltaSeconds() + 200.f;
			const FVector TraceStart = CauserLocation - TravelDirection * TraceBackDistance;

			FHitResult EntryHit;
			if (Collision->LineTraceComponent(EntryHit, TraceStart, CauserLocation, FCollisionQueryParams::DefaultQueryParam))
			{
				HitLocation = EntryHit.ImpactPoint;
				HitNormal = -TravelDirection; // 쏜 사람 쪽으로 튄다
				bFoundEntry = true;
			}
		}

		// 멈춰 있는 가해 액터(수류탄)이거나 경로를 못 찾았으면, 가해 액터 쪽 몸 표면을 쓴다.
		if (false == bFoundEntry)
		{
			if (nullptr != Collision)
			{
				FVector ClosestPoint;
				if (Collision->GetClosestPointOnCollision(CauserLocation, ClosestPoint) >= 0.f)
				{
					HitLocation = ClosestPoint;
				}
			}

			const FVector ToCauser = (CauserLocation - Avatar->GetActorLocation()).GetSafeNormal();
			if (false == ToCauser.IsNearlyZero())
			{
				HitNormal = ToCauser;
			}
		}
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = HitLocation;
	CueParameters.Normal = HitNormal;
	CueParameters.RawMagnitude = Damage;
	CueParameters.Instigator = Context.GetInstigator();
	CueParameters.EffectCauser = Causer;

	AbilitySystem->ExecuteGameplayCue(CueTag, CueParameters);
}

APlayerStateBase* UFPSHealthSet::FindDamagePlayerState(AActor* DamageSource) const
{
	if (false == IsValid(DamageSource))
	{
		return nullptr;
	}

	const UDamageSourceComponent* SourceComponent = DamageSource->FindComponentByClass<UDamageSourceComponent>();
	if (true == IsValid(SourceComponent))
	{
		return SourceComponent->GetSourcePlayerState();
	}

	APlayerStateBase* PlayerState = Cast<APlayerStateBase>(DamageSource);
	if (true == IsValid(PlayerState))
	{
		return PlayerState;
	}

	const AController* Controller = Cast<AController>(DamageSource);
	if (true == IsValid(Controller))
	{
		return Controller->GetPlayerState<APlayerStateBase>();
	}

	const ACharacterPlayer* Character = Cast<ACharacterPlayer>(DamageSource);
	if (true == IsValid(Character))
	{
		return Character->GetCombatPlayerState();
	}
	const APawn* Pawn = Cast<APawn>(DamageSource);
	if (false == IsValid(Pawn))
	{
		Pawn = DamageSource->GetInstigator();
	}
	return (true == IsValid(Pawn)) ? Pawn->GetPlayerState<APlayerStateBase>() : nullptr;
}

void UFPSHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSHealthSet, _Health, OldValue);
}

void UFPSHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSHealthSet, _MaxHealth, OldValue);
}

void UFPSHealthSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSHealthSet, _Shield, OldValue);
}

void UFPSHealthSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSHealthSet, _MaxShield, OldValue);
}
