#include "Component/Ability/GamePlayAbility/FPSChangeFireModeAbility.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Character/CharacterPlayer.h"
#include "Weapons/WeaponActor.h"
#include "GameTag/FPSGameplayTag.h"

UFPSChangeFireModeAbility::UFPSChangeFireModeAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Vault);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Mantle);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Reloading);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Melee);
}

bool UFPSChangeFireModeAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	const AWeaponActor* Weapon = IsValid(Character) ? Character->GetEquippedWeapon() : nullptr;
	return IsValid(Weapon) && Character->CanFireFromAbility() && Weapon->CanToggleFireMode()
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UFPSChangeFireModeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	UFPSAbilitySystemComponent* ASC = ActorInfo ? Cast<UFPSAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	bool Changed = false;
	if (IsValid(Character) && Character->HasAuthority() && IsValid(ASC) && Character->CanFireFromAbility()
		&& IsValid(Character->GetEquippedWeapon()) && Character->GetEquippedWeapon()->CanToggleFireMode()
		&& CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		ASC->CancelWeaponFire();
		Changed = Character->GetEquippedWeapon()->ToggleFireMode();
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !Changed);
}
