#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/GamePlayAbility/FPSCombatAbility.h"
#include "TimerManager.h"
#include "FPSFireAbility.generated.h"

class AWeaponActor;

// Server-authoritative firing session bound to the weapon equipped at activation.
UCLASS(Blueprintable)
class PROJECTFPS_API UFPSFireAbility : public UFPSCombatAbility
{
	GENERATED_BODY()

	UFPSFireAbility();
private:
	TWeakObjectPtr<AWeaponActor> _FiringWeapon;

	FTimerHandle _ShotTimer;

	TMap<FGameplayTag, FDelegateHandle> _BlockedTagDelegates;

	void FireNextShot();

	void OnBlockedTagChanged(FGameplayTag Tag, int32 Count);

public:
	/**
	 * 어빌리티의 사용 가능 여부를 미리 확인 
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
				 const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
				 const FGameplayTagContainer* TargetTags = nullptr,
				 FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * 어빌리티가 실제로 발동되어 수행할 핵심 로직을 정의하는 함수 
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;
	/**
	 * 실행을 마친 어빌리티를 안전하게 종료하는 핵심 함수
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility, bool bWasCancelled) override;
};
