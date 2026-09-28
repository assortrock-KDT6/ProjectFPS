// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Character/CharacterPlayer.h"
#include "Component/Ability/GameEffect/FPSMatchCombatBlockEffect.h"
#include "GameMode/FPSGameMode.h"
#include "GameMode/FPSGameState.h"
#include "GameTag/FPSGameplayTag.h"
#include "Component/Ability/GamePlayAbility/FPSFireAbility.h"
#include "Component/Ability/GamePlayAbility/FPSChangeFireModeAbility.h"

UFPSAbilitySystemComponent::UFPSAbilitySystemComponent()
{
	FireAbilityClass = UFPSFireAbility::StaticClass();
	ChangeFireModeAbilityClass = UFPSChangeFireModeAbility::StaticClass();
}

void UFPSAbilitySystemComponent::GrantWeaponAbilities()
{
	if (false == IsOwnerActorAuthoritative() || false == IsValid(Cast<ACharacterPlayer>(GetAvatarActor())))
	{
		return;
	}

	if (nullptr == FindAbilitySpecFromHandle(_FireAbilityHandle) && FireAbilityClass)
	{
		FGameplayAbilitySpec* Existing = FindAbilitySpecFromClass(FireAbilityClass);
		_FireAbilityHandle = Existing ? Existing->Handle : GiveAbility(FGameplayAbilitySpec(FireAbilityClass, 1));
	}

	if (nullptr == FindAbilitySpecFromHandle(_ChangeFireModeAbilityHandle) && nullptr != ChangeFireModeAbilityClass)
	{
		FGameplayAbilitySpec* Existing = FindAbilitySpecFromClass(ChangeFireModeAbilityClass);
		_ChangeFireModeAbilityHandle = Existing ? Existing->Handle : GiveAbility(FGameplayAbilitySpec(ChangeFireModeAbilityClass, 1));
	}
}

void UFPSAbilitySystemComponent::SetFireInput(bool Pressed)
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActor());
	
	if ((true == IsValid(Pawn)) && ((true == Pawn->HasAuthority()) || (true == Pawn->IsLocallyControlled())))
	{
		ServerSetFireInput(Pressed);
	}
}

void UFPSAbilitySystemComponent::ChangeFireMode()
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActor());
	if ((true == IsValid(Pawn)) && (true == (Pawn->HasAuthority()) || (true == Pawn->IsLocallyControlled())))
	{
		ServerChangeFireMode();
	}
}

void UFPSAbilitySystemComponent::ServerSetFireInput_Implementation(bool Pressed)
{
	if (false == IsOwnerActorAuthoritative())
	{
		return;
	}

	if (false == Pressed)
	{
		_FireInputHeld = false;
		CancelWeaponFire();
		return;
	}

	if (_FireInputHeld) 
	{
		return;
	}

	_FireInputHeld = true;
	
	TryActivateAbility(_FireAbilityHandle);
}

void UFPSAbilitySystemComponent::ServerChangeFireMode_Implementation()
{
	if (false == IsOwnerActorAuthoritative())
	{
		return;
	}

	TryActivateAbility(_ChangeFireModeAbilityHandle);
}

void UFPSAbilitySystemComponent::CancelWeaponFire()
{
	const FGameplayTagContainer FireTags(FPSGameplayTags::Ability_Combat_Fire);

	CancelAbilities(&FireTags);
}

void UFPSAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	GrantWeaponAbilities();
	
	if (false ==_CombatBlockedTagDelegate.IsValid())
	{
		_CombatBlockedTagDelegate = RegisterGameplayTagEvent(FPSGameplayTags::Status_Combat_Blocked).AddUObject(this, &UFPSAbilitySystemComponent::HandleCombatBlockedTagChanged);
	}
	if ((true == IsValid(InOwnerActor)) && (true == InOwnerActor->HasAuthority()))
	{
		const AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();

		SetMatchCombatBlocked(!IsValid(GameMode) || !GameMode->IsCombatAllowed());
	}
}

bool UFPSAbilitySystemComponent::CanAttack() const
{
	const AActor* Avatar = GetAvatarActor();
	if (false == IsValid(Avatar) || false == IsValid(GetWorld())
		|| true == HasMatchingGameplayTag(FPSGameplayTags::Status_Combat_Blocked)
		|| true == HasMatchingGameplayTag(FPSGameplayTags::Status_Death))
	{
		return false;
	}

	if (Avatar->HasAuthority())
	{
		const AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();
		return IsValid(GameMode) && GameMode->IsCombatAllowed();
	}

	// 최초 태그 복제 전에도 클라이언트가 대기 중 공격을 예측 실행하지 않는다.
	const AFPSGameState* GameState = GetWorld()->GetGameState<AFPSGameState>();
	return IsValid(GameState) && GameState->IsMatchInProgress() && GameState->GetRemainingMatchTime() > 0.f;
}

void UFPSAbilitySystemComponent::SetMatchCombatBlocked(bool Blocked)
{
	if (false == IsOwnerActorAuthoritative())
	{
		return;
	}

	if (true == Blocked)
	{
		if (nullptr == GetActiveGameplayEffect(_MatchCombatBlockEffectHandle))
		{
			_MatchCombatBlockEffectHandle = ApplyGameplayEffectToSelf(GetDefault<UFPSMatchCombatBlockEffect>(), 1.f, MakeEffectContext());
		}
	}
	else if (true == _MatchCombatBlockEffectHandle.IsValid())
	{
		RemoveActiveGameplayEffect(_MatchCombatBlockEffectHandle);
		_MatchCombatBlockEffectHandle.Invalidate();
	}
}

void UFPSAbilitySystemComponent::HandleCombatBlockedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	const FGameplayTagContainer CombatAbilities(FPSGameplayTags::Ability_Combat);

	CancelAbilities(&CombatAbilities);

	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActor());

	if (true == IsValid(Character))
	{
		Character->StopAttacking();
	}
}

void UFPSAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RegisterGameplayTagEvent(FPSGameplayTags::Status_Combat_Blocked).Remove(_CombatBlockedTagDelegate);

	_CombatBlockedTagDelegate.Reset();

	Super::EndPlay(EndPlayReason);
}

