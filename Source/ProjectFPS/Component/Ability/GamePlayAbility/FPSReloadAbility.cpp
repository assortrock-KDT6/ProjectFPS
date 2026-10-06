// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Ability/GamePlayAbility/FPSReloadAbility.h"
#include "GameTag/FPSGameplayTag.h"
#include "Weapons/WeaponActor.h"
#include "Character/CharacterPlayer.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Component/Ability/GamePlayAbility/FPSGrenadeAbility.h"
#include "Component/Ability/GamePlayAbility/FPSGrenadeAbility.h"
#include "Engine/World.h"

UFPSReloadAbility::UFPSReloadAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	SetAssetTags(FGameplayTagContainer(FPSGameplayTags::Ability_Combat_Reload));
	BlockAbilitiesWithTag.AddTag(FPSGameplayTags::Ability_Combat_Reload);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Reloading);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Melee);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Vault);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Mantle);
}

bool UFPSReloadAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (false == Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	const ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	
	if (false == IsValid(Character))
	{
		return false;
	}

	// 조건 검사에서는 상태를 변경하지 않는다. 무기는 활성화 시 저장한다.
	const AWeaponActor* Weapon = Character->GetEquippedWeapon();
	if (false == IsValid(Weapon) || Weapon->GetOwner() != Character
		|| false == Character->CanFireFromAbility() || false == Weapon->CanReload())
	{
		return false;
	}

	const UFPSAbilitySystemComponent* ASC = Cast<UFPSAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (IsValid(ASC) && ASC->GrenadeAbilityClass)
	{
		const FGameplayAbilitySpec* GrenadeSpec = ASC->FindAbilitySpecFromClass(ASC->GrenadeAbilityClass);
		if (GrenadeSpec && GrenadeSpec->IsActive())
		{
			return false;
		}
	}
	
	return true;
}

void UFPSReloadAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	UFPSAbilitySystemComponent* ASC = ActorInfo ? Cast<UFPSAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	AWeaponActor* Weapon = IsValid(Character) ? Character->GetEquippedWeapon() : nullptr;

	if (false == IsValid(Character) || false == Character->HasAuthority() || false == IsValid(ASC)
		|| false == IsValid(GetWorld()) || false == IsValid(Weapon) || Weapon->GetOwner() != Character
		|| false == Character->CanFireFromAbility() || false == Weapon->CanReload()
		|| false == CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	_ReloadingWeapon = Weapon;
	Character->StopAttacking();

	_ReloadStateApplied = true;
	Character->SetAnimationStateTag(FPSGameplayTags::Status_Reloading, true);

	// 재장전 중 사망/전투 금지/파쿠르/근접 공격 상태가 되면 취소한다.
	for (const FGameplayTag& Tag : ActivationBlockedTags)
	{
		if (Tag != FPSGameplayTags::Status_Reloading)
		{
			_BlockedTagDelegates.Add(Tag, ASC->RegisterGameplayTagEvent(Tag).AddUObject(this, &UFPSReloadAbility::OnBlockedTagChanged));
		}
	}

	// TODO: 무기별 1인칭/3인칭 리로드 몽타주 재생을 연결한다.
	GetWorld()->GetTimerManager().SetTimer(_ReloadTimer, this, &UFPSReloadAbility::CompleteReload, Weapon->GetReloadTime(), false);
}

void UFPSReloadAbility::CompleteReload()
{
	if (false == IsActive())
	{
		return;
	}

	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());
	UFPSAbilitySystemComponent* ASC = Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	AWeaponActor* Weapon = _ReloadingWeapon.Get();

	if (false == IsValid(Character) || false == Character->HasAuthority() || false == IsValid(ASC)
		|| false == ASC->CanAttack() || false == Character->CanFireFromAbility()
		|| false == IsValid(Weapon) || Character->GetEquippedWeapon() != Weapon || Weapon->GetOwner() != Character)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// TODO: 예비 탄약이 생기면 부족한 탄창 수와 보유량으로 충전량을 계산한다.
	const bool Reloaded = Weapon->ReloadAmmo(Weapon->GetMaxAmmo() - Weapon->GetCurrentAmmo());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !Reloaded);
}

void UFPSReloadAbility::OnBlockedTagChanged(FGameplayTag Tag, int32 Count)
{
	if (Count > 0 && IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UFPSReloadAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(_ReloadTimer);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		for (const auto& Entry : _BlockedTagDelegates)
		{
			ASC->RegisterGameplayTagEvent(Entry.Key).Remove(Entry.Value);
		}
	}
	_BlockedTagDelegates.Empty();

	if (_ReloadStateApplied)
	{
		if (ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo()))
		{
			Character->SetAnimationStateTag(FPSGameplayTags::Status_Reloading, false);
		}
	}
	_ReloadStateApplied = false;
	_ReloadingWeapon.Reset();

	// TODO: 취소 시 리로드 몽타주 정지를 연결한다. 탄약 충전은 완료 함수에서만 한다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
