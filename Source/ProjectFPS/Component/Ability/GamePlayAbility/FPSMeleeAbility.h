#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/GamePlayAbility/FPSCombatAbility.h"
#include "FPSMeleeAbility.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_WaitInputPress;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FFPSMeleeAttack
{
	GENERATED_BODY()

	// 로컬 플레이어의 팔과 다른 플레이어에게 보이는 몸을 각각 재생한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FirstPersonMontage;

	// 서버 판정 이벤트와 원격 플레이어 표시에는 몸 몽타주를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ThirdPersonMontage;

	// 한 타격에서 같은 대상에게 적용하는 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Damage = 35.f;

	// 1인칭 팔과 몸 몽타주에 동일하게 적용하는 재생 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float PlayRate = 2.f;
};

// 한 번의 예측 활성화로 전체 콤보를 관리하며, 실제 피해는 서버에서만 적용한다.
UCLASS()
class PROJECTFPS_API UFPSMeleeAbility : public UFPSCombatAbility
{
	GENERATED_BODY()

public:
	UFPSMeleeAbility();

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

	// 배열 순서가 콤보 순서이며, GA_MeleeBase의 클래스 기본값에서 조정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TArray<FFPSMeleeAttack> Attacks;

	// 단계별 피해량을 SetByCaller로 받아 공통 피해 처리 경로에 적용하는 효과이다.
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 시선 방향으로 검사하는 거리와 구체 반경이다. 단위는 cm이다.
	UPROPERTY(EditDefaultsOnly, Category = "Melee", meta = (ClampMin = "1"))
	float Reach = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee", meta = (ClampMin = "1"))
	float SweepRadius = 22.f;

	// 공격하지 않을 때는 INDEX_NONE이며, 공격 중에는 0부터 시작한다.
	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	int32 ComboIndex = INDEX_NONE;

	// 추가 입력은 횟수를 누적하지 않고 다음 공격 하나만 예약한다.
	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	bool bBufferedInput = false;

	// 콤보 노티파이가 알리는 현재 전환 구간이다. 선행 입력도 종료 전까지 예약할 수 있다.
	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	bool bComboWindowOpen = false;

private:
	void StartAttack(int32 Index);

	void WaitForComboInput();

	void SweepForTargets();

	void StopHitWindow();

	void FailAttack();

	void OnBlockedTagChanged(FGameplayTag Tag, int32 Count);

	UFUNCTION()
	void OnComboInput(float TimeWaited);

	UFUNCTION()
	void OnMeleeEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnAttackCompleted();

	UFUNCTION()
	void OnAttackInterrupted();

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> _MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> _EventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitInputPress> _InputTask;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> _PlayingFirstPersonMontage;

	// 타격 구간 동안 여러 번 검사하더라도 같은 대상은 한 번만 맞는다.
	TSet<TWeakObjectPtr<AActor>> _HitActors;

	TMap<FGameplayTag, FDelegateHandle> _BlockedTagDelegates;

	FTimerHandle _HitTimer;

	// 몽타주 종료 콜백이 누락될 경우 능력이 계속 유지되지 않도록 제한한다.
	FTimerHandle _WatchdogTimer;

	bool _InputWindowClosed = false;

	// 종료 과정에서 발생하는 몽타주 콜백의 재진입을 막는다.
	bool _Ending = false;
};
