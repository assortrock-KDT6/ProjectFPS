#include "Component/Ability/DamageSourceComponent.h"
#include "GameMode/PlayerStateBase.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

void UDamageSourceComponent::InitializeDamageWeapon(const FDamageWeaponInfo& WeaponInfo)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || _WeaponInfoInitialized)
	{
		return;
	}
	_DamageWeapon = WeaponInfo;
	_WeaponInfoInitialized = true;
}

FDamageWeaponInfo UDamageSourceComponent::ResolveDamageWeapon(const FGameplayEffectContextHandle& Context)
{
	if (const AActor* Causer = Context.GetEffectCauser(); IsValid(Causer))
	{
		if (const UDamageSourceComponent* Source = Causer->FindComponentByClass<UDamageSourceComponent>())
		{
			return Source->GetDamageWeapon();
		}
	}
	if (const UDamageSourceComponent* Source = Cast<UDamageSourceComponent>(Context.GetSourceObject()))
	{
		return Source->GetDamageWeapon();
	}
	if (const AActor* SourceActor = Cast<AActor>(Context.GetSourceObject()); IsValid(SourceActor))
	{
		if (const UDamageSourceComponent* Source = SourceActor->FindComponentByClass<UDamageSourceComponent>())
		{
			return Source->GetDamageWeapon();
		}
	}
	// 근접/환경 피해 등 출처가 없는 공격에 현재 장착 무기를 대신 표시하지 않는다.
	return FDamageWeaponInfo();
}

void UDamageSourceComponent::SetSourcePlayerState(APlayerStateBase* PlayerState)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		_SourcePlayerState = PlayerState;
	}
}

APlayerStateBase* UDamageSourceComponent::GetSourcePlayerState() const
{
	return _SourcePlayerState.Get();
}

void UDamageSourceComponent::InitializeProjectileDamage(float Damage)
{
	if (false == IsValid(GetOwner()) || false == GetOwner()->HasAuthority() || true == _DamageInitialized)
	{
		return;
	}

	_ProjectileDamage = FMath::IsFinite(Damage) ? FMath::Max(0.f, Damage) : 0.f;

	_DamageInitialized = true;
}

bool UDamageSourceComponent::ConsumeProjectileHit(AActor* Projectile, AActor* Target, float& Damage)
{
	Damage = 0.f;

	if (false == IsValid(Projectile) || false == Projectile->HasAuthority() || false == IsValid(Target)
		|| Target == Projectile || Target == Projectile->GetOwner() || Target == Projectile->GetInstigator()
		|| false == IsValid(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target)))
	{
		return false;
	}

	UDamageSourceComponent* Source = Projectile->FindComponentByClass<UDamageSourceComponent>();

	if (false == IsValid(Source) || false == Source->_DamageInitialized || true == Source->_HitConsumed || Source->_ProjectileDamage <= 0.f)
	{
		return false;
	}

	// Overlaps can re-enter while applying effects. Consume before invoking gameplay code.
	Source->_HitConsumed = true;

	Damage = Source->_ProjectileDamage;

	return true;
}
