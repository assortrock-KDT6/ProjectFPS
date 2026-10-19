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

	void InitializeProjectileDamage(float Damage);

	void InitializeDamageWeapon(const FDamageWeaponInfo& WeaponInfo);
	const FDamageWeaponInfo& GetDamageWeapon() const { return _DamageWeapon; }
	static FDamageWeaponInfo ResolveDamageWeapon(const FGameplayEffectContextHandle& Context);


};
