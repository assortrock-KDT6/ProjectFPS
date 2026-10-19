#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/WeaponTypes.h"
#include "DamageSourceComponent.generated.h"

class APlayerStateBase;
struct FGameplayEffectContextHandle;

/** 
 *발사자의 Pawn이 파괴되어도 서버가 투사체의 처치 기록을 귀속할 수 있게 한다. 
 */
UCLASS()
class PROJECTFPS_API UDamageSourceComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	TWeakObjectPtr<APlayerStateBase> _SourcePlayerState;
	float _ProjectileDamage = 0.f;
	bool _DamageInitialized = false;
	bool _HitConsumed = false;
	bool _WeaponInfoInitialized = false;
	FVector _IncomingHitDirection = FVector::ZeroVector;
	FVector _ShotOrigin = FVector::ZeroVector;

	UPROPERTY(Transient)
	FDamageWeaponInfo _DamageWeapon;

public:
	UFUNCTION(BlueprintPure, Category = "Combat|Projectile")
	float GetProjectileDamage() const 
	{ 
		return _ProjectileDamage;
	}

	// Claims one valid server hit before Blueprint applies the damage spec.
	UFUNCTION(BlueprintCallable, Category = "Combat|Projectile")
	static bool ConsumeProjectileHit(AActor* Projectile, AActor* Target, float& Damage);

public:
	void SetSourcePlayerState(APlayerStateBase* PlayerState);

	APlayerStateBase* GetSourcePlayerState() const;

	// 지연 생성 중에는 루트가 없을 수 있으므로 무기에서 계산한 발사 방향을 직접 받는다.
	void InitializeProjectileDamage(float Damage, const FVector& FireDirection);

	void InitializeDamageWeapon(const FDamageWeaponInfo& WeaponInfo);
	const FDamageWeaponInfo& GetDamageWeapon() const { return _DamageWeapon; }
	static FDamageWeaponInfo ResolveDamageWeapon(const FGameplayEffectContextHandle& Context);

	// 총알의 입사 방향만 반환한다. 수류탄류와 출처가 없는 피해는 방향을 표시하지 않는다.
	static FVector ResolveIncomingDirection(const FGameplayEffectContextHandle& Context, const AActor* Victim);

	// 피격 순간의 공격자와 피해자 사이 거리. 공격자가 사라졌으면 발사 당시 위치를 사용한다.
	// 총알의 피해 출처를 확인할 수 없으면 -1을 반환한다.
	static float ResolveSourceDistance(const FGameplayEffectContextHandle& Context, const AActor* Victim);


};
