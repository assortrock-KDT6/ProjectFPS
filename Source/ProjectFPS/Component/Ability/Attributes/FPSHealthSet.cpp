// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "UI/GamePlay/PlayerStatusWidget.h"
#include "UI/GameHUD.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

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
				const float NewHealth = FMath::Clamp(CurrentHealth, CurrentHealth - DamageToApply, 0.f);
				Set_Health(NewHealth);

				if (NewHealth <= 0.f)
				{
					// TODO : 캐릭터 사망 이벤트.

				}

			}
		}
	}
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
