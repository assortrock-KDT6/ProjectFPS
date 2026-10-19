// 프로젝트 설정의 설명 페이지에 저작권 정보를 입력하세요.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "FPSAbilitySystemComponent.generated.h"

struct FGameplayTag;
struct FGameplayTagContainer;
class UFPSFireAbility;
class UFPSChangeFireModeAbility;
class UFPSGrenadeAbility;
class UFPSReloadAbility;
class UFPSMeleeAbility;

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

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Abilities")
	TSubclassOf<UFPSReloadAbility> ReloadAbilityClass;

	// GA_MeleeBase에서 콤보 몽타주와 단계별 피해량을 조정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Abilities")
	TSubclassOf<UFPSMeleeAbility> MeleeAbilityClass;

private:
	// 시작 시 즉시 로드하지 않아도 쿠커가 기본 능력 블루프린트를 포함하도록 참조를 보관한다.
	UPROPERTY()
	TSoftClassPtr<UFPSMeleeAbility> _DefaultMeleeBlueprint;

	FGameplayAbilitySpecHandle _FireAbilityHandle;
	
	FGameplayAbilitySpecHandle _ChangeFireModeAbilityHandle;
	
	FGameplayAbilitySpecHandle _GrenadeAbilityHandle;

	FGameplayAbilitySpecHandle _ReloadAbilityHandle;

	FGameplayAbilitySpecHandle _MeleeAbilityHandle;

	FActiveGameplayEffectHandle _MatchCombatBlockEffectHandle;
	
	FDelegateHandle _CombatBlockedTagDelegate;
	
	bool _FireInputHeld = false;
public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 입력은 순서와 전달을 보장하여 전송하고, 발사 시점 관리는 GA_Fire에서 담당한다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void SetFireInput(bool Pressed);

	// 첫 입력은 능력을 활성화하고, 공격 중 추가 입력은 콤보 예약으로 전달한다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void SetMeleeInput(bool Pressed);

	// 부여된 근접 공격 능력의 인스턴스를 조회한다. 활성화 전에는 없을 수 있다.
	UFUNCTION(BlueprintPure, Category = "Combat|Melee")
	UFPSMeleeAbility* GetMeleeAbilityInstance() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void CookGrenade();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void ChangeFireMode();

	// 입력이나 Blueprint에서 호출하는 재장전 시작 함수.
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanAttack() const;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetFireInput(bool Pressed);

	UFUNCTION(Server, Reliable)
	void ServerChangeFireMode();

	UFUNCTION(Server, Reliable)
	void ServerCookGrenade();

	UFUNCTION(Server, Reliable)
	void ServerReload();
	
public:
	void CancelWeaponFire();
	void CancelWeaponReload();

	// GameMode만 매치 소유 효과를 추가/제거한다. 다른 효과의 동일 태그는 유지한다.
	void SetMatchCombatBlocked(bool Blocked);

	void GrantWeaponAbilities();

	void HandleCombatBlockedTagChanged(const FGameplayTag Tag, int32 NewCount);
};
