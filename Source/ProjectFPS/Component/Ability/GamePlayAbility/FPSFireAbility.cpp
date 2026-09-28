#include "Component/Ability/GamePlayAbility/FPSFireAbility.h"
#include "Character/CharacterPlayer.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "GameTag/FPSGameplayTag.h"
#include "Weapons/WeaponActor.h"
#include "Engine/World.h"

UFPSFireAbility::UFPSFireAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	SetAssetTags(FGameplayTagContainer(FPSGameplayTags::Ability_Combat_Fire));
	BlockAbilitiesWithTag.AddTag(FPSGameplayTags::Ability_Combat_Fire);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Vault);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Mantle);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Reloading);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Melee);
}

bool UFPSFireAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	const AWeaponActor* Weapon = IsValid(Character) ? Character->GetEquippedWeapon() : nullptr;

	return true == IsValid(Weapon) && true == Character->CanFireFromAbility()
		&& Weapon->SupportsFireMode(Weapon->GetFireMode()) && Weapon->GetProjectileInterval() > 0.f
		&& Weapon->GetRemainingFireInterval() <= 0.
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UFPSFireAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (false == IsValid(Character) || false == Character->HasAuthority() || false == Character->CanFireFromAbility()
		|| false == CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	_FiringWeapon = Character->GetEquippedWeapon();

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	for (const FGameplayTag& Tag : ActivationBlockedTags)
	{
		_BlockedTagDelegates.Add(Tag, AbilitySystemComponent->RegisterGameplayTagEvent(Tag).AddUObject(this, &UFPSFireAbility::OnBlockedTagChanged));
	}
	FireNextShot();
}

void UFPSFireAbility::FireNextShot()
{
	if (false == IsActive())
	{
		return;
	}

	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());

	AWeaponActor* Weapon = _FiringWeapon.Get();

	UFPSAbilitySystemComponent* AbilitySystemComponent = Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());

	if (false == IsValid(Character) || false == IsValid(Weapon) || false == IsValid(AbilitySystemComponent) || false == AbilitySystemComponent->CanAttack()
		|| true == AbilitySystemComponent->HasAnyMatchingGameplayTags(ActivationBlockedTags) || false == Character->CanFireFromAbility()
		|| Character->GetEquippedWeapon() != Weapon || Weapon->GetOwner() != Character)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// Timers may wake slightly early; retain the weapon's interval across activations.
	const double Remaining = Weapon->GetRemainingFireInterval();

	if (Remaining > 0.)
	{
		GetWorld()->GetTimerManager().SetTimer(_ShotTimer, this, &UFPSFireAbility::FireNextShot, FMath::Max(static_cast<float>(Remaining), 0.001f), false);
		return;
	}

	const bool Fired = Weapon->Fire(Character->GetAimPoint(Weapon->GetWeaponRange()));

	if (true == Fired)
	{
		Character->NotifyAbilityWeaponFired(Weapon);
	}

	if (false == IsActive())
	{
		return;
	}

	if (false == Fired || EWeaponFireMode::Automatic != Weapon->GetFireMode() )
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !Fired);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(_ShotTimer, this, &UFPSFireAbility::FireNextShot,FMath::Max(Weapon->GetProjectileInterval(), 0.001f), false);
}

void UFPSFireAbility::OnBlockedTagChanged(FGameplayTag Tag, int32 Count)
{
	if (Count > 0 && true == IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UFPSFireAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (nullptr != GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(_ShotTimer);
	}

	if (UAbilitySystemComponent* AbilitySyatemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		for (const auto& Entry : _BlockedTagDelegates)
		{
			AbilitySyatemComponent->RegisterGameplayTagEvent(Entry.Key).Remove(Entry.Value);
		}
	}

	_BlockedTagDelegates.Empty();

	_FiringWeapon.Reset();

	if (true == bWasCancelled)
	{
		if (ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo()))
		{
			Character->StopFiringPresentation();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
