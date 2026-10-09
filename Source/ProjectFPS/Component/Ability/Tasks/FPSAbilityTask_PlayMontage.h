#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "FPSAbilityTask_PlayMontage.generated.h"

class ACharacterPlayer;
class UAnimMontage;

// 1인칭 표시는 소유 클라이언트에 전달한다. 리로드 완료 판정은 Ability가 담당한다.
UCLASS()
class PROJECTFPS_API UFPSAbilityTask_PlayMontage : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ability | Tasks", meta = (HidePin = "OwningAbility",
		DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UFPSAbilityTask_PlayMontage* PlayMontage(
		UGameplayAbility* OwningAbility, UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float FPPlayRate = 1.f, float TPPlayRate = 1.f,
		bool bStopWhenAbilityEnds = true);

	virtual void Activate() override;

protected:
	virtual void OnDestroy(bool bAbilityEnded) override;

private:
	void OnAbilityCancelled();
	FDelegateHandle _AbilityCancelledHandle;

	UPROPERTY()
	TObjectPtr<UAnimMontage> _FistPersonMontage = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimMontage> _ThirdPersonMontage = nullptr;

	TWeakObjectPtr<ACharacterPlayer> _Character;

	float _FPPlayRate		= 1.f;
	float _TPPlayRate		= 1.f;

	bool _StopWhenAbilityEnds = true;
	bool _PlayRequested = false;
};
