// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/GamePlayAbility/FPSCombatAbility.h"
#include "TimerManager.h"
#include "FPSReloadAbility.generated.h"


class AWeaponActor;
// 재장전 진행은 Ability, 실제 탄약 변경은 WeaponActor가 담당한다.
UCLASS()
class PROJECTFPS_API UFPSReloadAbility : public UFPSCombatAbility
{
	GENERATED_BODY()
	
private:
	TWeakObjectPtr<AWeaponActor> _ReloadingWeapon;
	FTimerHandle _ReloadTimer;
	TMap<FGameplayTag, FDelegateHandle> _BlockedTagDelegates;
	bool _ReloadStateApplied = false;

	void CompleteReload();
	void OnBlockedTagChanged(FGameplayTag Tag, int32 Count);

public:
	UFPSReloadAbility();

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
