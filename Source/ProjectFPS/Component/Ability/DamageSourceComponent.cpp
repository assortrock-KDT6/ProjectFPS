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

void UDamageSourceComponent::InitializeProjectileDamage(float Damage, const FVector& FireDirection)
{
	if (false == IsValid(GetOwner()) || false == GetOwner()->HasAuthority() || true == _DamageInitialized)
	{
		return;
	}

	_ProjectileDamage = FMath::IsFinite(Damage) ? FMath::Max(0.f, Damage) : 0.f;
	// FinishSpawning 이전에도 총알의 실제 발사 방향을 보관한다.
	// 명중 BP가 먼저 이동을 정지해 속도가 0이 되어도 이 방향을 사용할 수 있다.
	_IncomingHitDirection = true == FireDirection.ContainsNaN() ? FVector::ZeroVector : -FireDirection.GetSafeNormal();
	const APawn* SourcePawn = GetOwner()->GetInstigator();
	_ShotOrigin = true == IsValid(SourcePawn) ? SourcePawn->GetActorLocation() : GetOwner()->GetActorLocation();

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
	const FVector Velocity = Projectile->GetVelocity();
	if (false == Velocity.ContainsNaN() && false == Velocity.IsNearlyZero())
	{
		Source->_IncomingHitDirection = -Velocity.GetSafeNormal();
	}

	Damage = Source->_ProjectileDamage;

	return true;
}

FVector UDamageSourceComponent::ResolveIncomingDirection(const FGameplayEffectContextHandle& Context, const AActor* Victim)
{
	if (false == IsValid(Victim))
	{
		return FVector::ZeroVector;
	}

	const AActor* Causer = Context.GetEffectCauser();
	if (true == IsValid(Causer) && Causer != Victim)
	{
		// 무기 발사 경로에서 피해량을 초기화하고 명중을 확정한 총알만 표시한다.
		// 수류탄과 불 장판의 DamageSource는 처치 기록만 보관하므로 이 조건에 해당하지 않는다.
		const UDamageSourceComponent* Source = Causer->FindComponentByClass<UDamageSourceComponent>();
		if (true == IsValid(Source) && true == Source->_DamageInitialized && true == Source->_HitConsumed)
		{
			return Source->_IncomingHitDirection.ContainsNaN()
				? FVector::ZeroVector : Source->_IncomingHitDirection.GetSafeNormal();
		}
	}

	// 폭발 위치나 화염 위치로 원형 방향을 만들지 않고 테두리만 표시한다.
	return FVector::ZeroVector;
}

float UDamageSourceComponent::ResolveSourceDistance(const FGameplayEffectContextHandle& Context, const AActor* Victim)
{
	const AActor* Causer = Context.GetEffectCauser();
	if (false == IsValid(Victim) || false == IsValid(Causer))
	{
		return -1.f;
	}

	const UDamageSourceComponent* Source = Causer->FindComponentByClass<UDamageSourceComponent>();
	if (false == IsValid(Source) || false == Source->_DamageInitialized || false == Source->_HitConsumed)
	{
		return -1.f;
	}

	// 명중한 총알 자체는 피해자 바로 옆에 있으므로 거리 계산에 사용하지 않는다.
	// PlayerState의 새 폰을 조회하지 않아 리스폰한 공격자의 위치가 섞이지 않게 한다.
	const APawn* SourcePawn = Causer->GetInstigator();
	const FVector SourceLocation = true == IsValid(SourcePawn) ? SourcePawn->GetActorLocation() : Source->_ShotOrigin;
	const float Distance = FVector::Distance(SourceLocation, Victim->GetActorLocation());
	return true == FMath::IsFinite(Distance) ? Distance : -1.f;
}
