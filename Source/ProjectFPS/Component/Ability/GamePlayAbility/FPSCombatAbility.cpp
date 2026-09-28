#include "Component/Ability/GamePlayAbility/FPSCombatAbility.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "GameTag/FPSGameplayTag.h"

UFPSCombatAbility::UFPSCombatAbility()
{
	SetAssetTags(FGameplayTagContainer(FPSGameplayTags::Ability_Combat));
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Combat_Blocked);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Death);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UFPSCombatAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UFPSAbilitySystemComponent* AbilitySystem = ActorInfo != nullptr ? Cast<UFPSAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (false == IsValid(AbilitySystem) || false == AbilitySystem->CanAttack())
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(FPSGameplayTags::Ability_ActiveteFail_TagsBlocked);
		}
		return false;
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
