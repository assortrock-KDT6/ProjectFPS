#include "Component/Ability/GameEffect/FPSMeleeDamageEffect.h"

#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "GameTag/FPSGameplayTag.h"

UFPSMeleeDamageEffect::UFPSMeleeDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 체력·실드·사망 처리는 HealthSet의 공통 피해 처리 경로를 사용한다.
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UFPSHealthSet::Get_DamageInAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	// 콤보 단계마다 다른 피해량을 SetByCaller.Damage로 전달할 수 있다.
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = FPSGameplayTags::SetByCaller_Damage;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
