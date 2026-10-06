// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/GamePlayAbility/FPSCombatAbility.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FPSGrenadeAbility.generated.h"

/**
 *  플레이어의 수류탄 투척 행동을 담당
 *  사망, 전투 금지 검사는 FPSCombatAbility의기존 구현을 재사용
 */
UCLASS(Blueprintable)
class PROJECTFPS_API UFPSGrenadeAbility : public UFPSCombatAbility
{
	GENERATED_BODY()
public:
	UFPSGrenadeAbility();
	
	// GAS가 이 능력을 활성화했을때 호출하는 시작 지점
	// 이후 이 함수에서 투척 준비와 입력 대기를 연결
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	// Ability System Component가 입력 해제를 전달하면 Throw로 전환
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// GA_Grenade에 지정된 몽타주를 장착 표시에서도 사용
	UAnimMontage* GetGrenadeMontageFP() const;

	UAnimMontage* GetGrenadeMontageTP() const;

protected:
	// Blueprint 기본값에서 지정할 1인칭 수류탄 몽타쥬
	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Animation")
	TObjectPtr<UAnimMontage> _GrenadeMontageFP = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Animation")
	TObjectPtr<UAnimMontage> _GrenadeMontageTP = nullptr;

private:
	// TP는 표시만 담당한다. 투척 노티파이와 능력 종료는 기존 FP가 결정한다.
	void PlayThirdPersonMontage(FName Section);
	bool _ThirdPersonMontageStarted = false;

	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	UFUNCTION()
	void OnGrenadeRelease(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
	
	// 서버에서 쿠킹 쵸청을 받아서 폭발하는 타이머 시작
	UFUNCTION()
	void OnGrenadeCook(FGameplayEventData Payload);
};
