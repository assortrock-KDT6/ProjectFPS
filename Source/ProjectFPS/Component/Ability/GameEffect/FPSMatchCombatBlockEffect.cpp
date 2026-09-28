#include "Component/Ability/GameEffect/FPSMatchCombatBlockEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/BlockAbilityTagsGameplayEffectComponent.h"
#include "GameTag/FPSGameplayTag.h"

UFPSMatchCombatBlockEffect::UFPSMatchCombatBlockEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* GrantedTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
	
	GEComponents.Add(GrantedTags);
	
	FInheritedTagContainer StateTags;
	
	StateTags.Added.AddTag(FPSGameplayTags::Status_Combat_Blocked);
	
	StateTags.Added.AddTag(FPSGameplayTags::Status_Damage_Immune);
	
	GrantedTags->SetAndApplyTargetTagChanges(StateTags);

	UBlockAbilityTagsGameplayEffectComponent* BlockedAbilities = CreateDefaultSubobject<UBlockAbilityTagsGameplayEffectComponent>(TEXT("BlockedAbilities"));
	
	GEComponents.Add(BlockedAbilities);
	
	FInheritedTagContainer AbilityTags;
	
	AbilityTags.Added.AddTag(FPSGameplayTags::Ability_Combat);
	
	BlockedAbilities->SetAndApplyBlockedAbilityTagChanges(AbilityTags);
}
