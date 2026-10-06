// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Character/CharacterPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/Ability/GameEffect/FPSMatchCombatBlockEffect.h"
#include "GameMode/FPSGameMode.h"
#include "GameMode/FPSGameState.h"
#include "GameTag/FPSGameplayTag.h"
#include "Component/Ability/GamePlayAbility/FPSFireAbility.h"
#include "Component/Ability/GamePlayAbility/FPSReloadAbility.h"
#include "Component/Ability/GamePlayAbility/FPSChangeFireModeAbility.h"
#include "Component/Ability/GamePlayAbility/FPSGrenadeAbility.h"	// 수류탄 행동을 처리하는 능력
#include "Projectiles/GrenadeActor.h"								// 실제로 손에 들고 있는 수류탄 액터

UFPSAbilitySystemComponent::UFPSAbilitySystemComponent()
{
	FireAbilityClass = UFPSFireAbility::StaticClass();
	ChangeFireModeAbilityClass = UFPSChangeFireModeAbility::StaticClass();
	ReloadAbilityClass = UFPSReloadAbility::StaticClass();
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
	
	// 이미 부여한 수류탄 능력이 있으면 재사용하고, 없으면 부여하기
	if (nullptr == FindAbilitySpecFromHandle(_GrenadeAbilityHandle) && nullptr != GrenadeAbilityClass)
	{
		FGameplayAbilitySpec* Existing = FindAbilitySpecFromClass(GrenadeAbilityClass);
		_GrenadeAbilityHandle = Existing ? Existing->Handle : GiveAbility(FGameplayAbilitySpec(GrenadeAbilityClass, 1));
	}

	if (nullptr == FindAbilitySpecFromHandle(_ReloadAbilityHandle) && ReloadAbilityClass)
	{
		FGameplayAbilitySpec* Existing = FindAbilitySpecFromClass(ReloadAbilityClass);
		_ReloadAbilityHandle = Existing ? Existing->Handle : GiveAbility(FGameplayAbilitySpec(ReloadAbilityClass, 1));
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

void UFPSAbilitySystemComponent::CookGrenade()
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActor());

	if (false == IsValid(Pawn))
	{
		return;
	}

	if (true == Pawn->HasAuthority() || true == Pawn->IsLocallyControlled())
	{
		ServerCookGrenade();
	}
}

void UFPSAbilitySystemComponent::ServerCookGrenade_Implementation()
{
	if (false == IsOwnerActorAuthoritative() || false == CanAttack())
	{
		return;
	}

	// 준비 중인 수류탄 능력이 있을 때만 쿠킹 요청을 전달한다.
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(_GrenadeAbilityHandle);

	if (nullptr == Spec || false == Spec->IsActive())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = FPSGameplayTags::Event_Grenade_Cook;
	EventData.Instigator = GetAvatarActor();
	EventData.Target = GetAvatarActor();

	HandleGameplayEvent(EventData.EventTag, &EventData);
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
		
		// 수류탄 능력에도 입력을 놓았다는 사실을 전달하기
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(_GrenadeAbilityHandle))
		{
			AbilitySpecInputReleased(*Spec);
		}
		
		return;
	}

	if (_FireInputHeld) 
	{
		return;
	}

	_FireInputHeld = true;
	
	const ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActor());
	if (false == IsValid(Character))
	{
		return;
	}
	
	if (IsValid(Character->GetEquippedGrenade()))
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(_GrenadeAbilityHandle))
		{
			Spec->InputPressed = true;
			
			TryActivateAbility(_GrenadeAbilityHandle);
		}
		
		// 수류탄을 들었을 때는 총기 능력을 실행하지 않는다.
		return;
	}
	
	// 손에서 수류탄이 떠났어도 Throw가 끝날때까지 총기발사를 시작하지 않는다
	if (const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(_GrenadeAbilityHandle))
	{
		if (Spec->IsActive())
		{
			return;
		}
	}
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

void UFPSAbilitySystemComponent::Reload()
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActor());
	if (IsValid(Pawn) && (Pawn->HasAuthority() || Pawn->IsLocallyControlled()))
	{
		ServerReload();
	}
}

void UFPSAbilitySystemComponent::ServerReload_Implementation()
{
	if (false == IsOwnerActorAuthoritative())
	{
		return;
	}
	TryActivateAbility(_ReloadAbilityHandle);
}

void UFPSAbilitySystemComponent::CancelWeaponFire()
{
	const FGameplayTagContainer FireTags(FPSGameplayTags::Ability_Combat_Fire);

	CancelAbilities(&FireTags);
}

void UFPSAbilitySystemComponent::CancelWeaponReload()
{
	if (IsOwnerActorAuthoritative())
	{
		CancelAbilityHandle(_ReloadAbilityHandle);
	}
}

void UFPSAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	// GAS 몽타주는 몸에서 재생한다. 1인칭 팔은 별도 표시/투척 판정 경로를 사용한다.
	if (const ACharacterPlayer* Character = Cast<ACharacterPlayer>(InAvatarActor))
	{
		AbilityActorInfo->SkeletalMeshComponent = Character->GetMesh();
	}
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

