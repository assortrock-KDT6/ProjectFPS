#include "Component/Ability/DamageSourceComponent.h"
#include "GameMode/PlayerStateBase.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

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
