// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/GamePlayAbility/FPSCombatAbility.h"
#include "Weapons/WeaponTypes.h"
#include "FPSReloadAbility.generated.h"


class AWeaponActor;
class UAnimMontage;
// 재장전 진행은 Ability, 실제 탄약 변경은 WeaponActor가 담당한다.
UCLASS(Blueprintable)
class PROJECTFPS_API UFPSReloadAbility : public UFPSCombatAbility
{
	GENERATED_BODY()
	
protected:
	// GA_Reload의 클래스 기본값에서 지정한다. 진행 시간은 무기의 _ReloadTime을 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Reload | Animation")
	TMap<EWeaponType, TObjectPtr<UAnimMontage>> _ReloadMontageFP;

	UPROPERTY(EditDefaultsOnly, Category = "Reload | Animation")
	TMap<EWeaponType, TObjectPtr<UAnimMontage>> _ReloadMontageTP;

private:
	TWeakObjectPtr<AWeaponActor> _ReloadingWeapon;
	
	TMap<FGameplayTag, FDelegateHandle> _BlockedTagDelegates;

	bool _ReloadStateApplied = false;

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
private:
	float GetReloadMontagePlayRate(const UAnimMontage* Montage, float ReloadTime) const;

	void StartReloadTasks(AWeaponActor* Weapon);

	UFUNCTION()
	void CompleteReload();

	void OnBlockedTagChanged(FGameplayTag Tag, int32 Count);

};
