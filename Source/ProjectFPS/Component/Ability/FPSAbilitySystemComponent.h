// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "FPSAbilitySystemComponent.generated.h"

struct FGameplayTag;
struct FGameplayTagContainer;
class UFPSFireAbility;
class UFPSChangeFireModeAbility;
class UFPSGrenadeAbility;
/**
 * 모든 전투 기능을 처리하는 거대한 컴포넌트를 만들면 안된다.
 * 
 * 해당 어빌리티 컴포넌트는 Actor 당 하나만 가질 수 있다.
 * 
 * 개별 사격 로직 -> GA_Fire
 * 재장전 로직 -> GA_Reload
 * 피해 공식 -> GameplayEffectExecutionCalculation
 * 체력 제한과 사망 판정 -> AttributeSet
 * 탄창과 무기 상태 -> Weapon / Equipment Component
 * 실제 이동 처리 -> Character Movement Component
 * 파티클.사운드 -> Gameplay Cue
 * 
 * 캐릭터가 가진 능리겨, 상태, 태그를 관리한다.
 */

UCLASS()
class PROJECTFPS_API UFPSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFPSAbilitySystemComponent();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Abilities")
	TSubclassOf<UFPSFireAbility> FireAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Abilities")
	TSubclassOf<UFPSChangeFireModeAbility> ChangeFireModeAbilityClass;
	
	// 블루프린트에서 실제 사용할 수류탄 능력을 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Abilities")
	TSubclassOf<UFPSGrenadeAbility> GrenadeAbilityClass;

private:
	FGameplayAbilitySpecHandle _FireAbilityHandle;
	
	FGameplayAbilitySpecHandle _ChangeFireModeAbilityHandle;
	
	FGameplayAbilitySpecHandle _GrenadeAbilityHandle;
	
	FActiveGameplayEffectHandle _MatchCombatBlockEffectHandle;
	
	FDelegateHandle _CombatBlockedTagDelegate;
	
	bool _FireInputHeld = false;
public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Reliable, ordered input transport. Shot scheduling belongs to GA_Fire.
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void SetFireInput(bool Pressed);

	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void CookGrenade();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void ChangeFireMode();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanAttack() const;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetFireInput(bool Pressed);
	UFUNCTION(Server, Reliable)
	void ServerChangeFireMode();
	UFUNCTION(Server, Reliable)
	void ServerCookGrenade();
	
public:
	void CancelWeaponFire();

	// GameMode만 매치 소유 효과를 추가/제거한다. 다른 효과의 동일 태그는 유지한다.
	void SetMatchCombatBlocked(bool Blocked);

	void GrantWeaponAbilities();

	void HandleCombatBlockedTagChanged(const FGameplayTag Tag, int32 NewCount);
};
