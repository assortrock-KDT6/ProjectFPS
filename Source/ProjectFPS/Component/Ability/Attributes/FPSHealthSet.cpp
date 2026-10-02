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

			if (CurrentShield > 0.f)
			{
				if (CurrentShield >= DamageToApply)
				{
					Set_Shield(CurrentShield - DamageToApply);
					DamageToApply = 0.f;
				}
				else
				{
					DamageToApply -= CurrentShield;
					Set_Shield(0.f);
				}
			}

			if (DamageToApply > 0.f)
			{
				const float NewHealth = FMath::Clamp(CurrentHealth - DamageToApply, 0.f, Get_MaxHealth());
				Set_Health(NewHealth);
				
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
