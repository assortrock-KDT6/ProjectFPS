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
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
				 const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
				 const FGameplayTagContainer* TargetTags = nullptr,
				 FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility, bool bWasCancelled) override;
};
