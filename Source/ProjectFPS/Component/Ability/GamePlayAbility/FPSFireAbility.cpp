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

		// 서버에서 연출을 쵸청하고, 네트워크 관련성에 따라
		// 해당 액터의 연출을 받을 클라이언트에 전당한다.

		// GAS가 제공하는 연출 정보 묶음-> 누가 발생시켰고, 무엇과 관련됐고, 어디서 발생했는지 담습니다.
		FGameplayCueParameters CueParameters;
		
		// 연출을 발생시킨 주제를 지정합니다. 여기서는 총을 쏜 캐릭터 입니다.
		CueParameters.Instigator = Character;

		// 연출의 출처로 사용할 객체를 지정합니다. 여기서는 발사한 무기입니다.
		CueParameters.SourceObject = Weapon;

		// 총구 소켓에 연출을 붙이기 위해 무기 메시를 넘김.
		// Location은 넘기지 않는다. 비워두면 GC에 지정한 소켓(Muzzle)의 위치를 사용한다.
		// 넘기면 나이아가라가 그 값을 소켓 기준 상대 오프셋으로 써서 총구에서 멀리 밀려난다.
		CueParameters.TargetAttachComponent = Weapon->GetWeaponMeshComponent();

		// 어떤 연출을 실행할지 지정하는 태그, 누가 무엇으로 어디서 발생할지에 대한 정보.
		AbilitySystemComponent->ExecuteGameplayCue(FPSGameplayTags::GameplayCue_Weapon_Fire, CueParameters);



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
