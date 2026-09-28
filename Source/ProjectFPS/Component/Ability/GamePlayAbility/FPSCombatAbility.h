#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/GamePlayAbility/FPSGameAbility.h"
#include "FPSCombatAbility.generated.h"

/** 사격/근접 공격 GA의 공통 부모. 이동/회복 GA는 FPSGameAbility를 사용한다. */
UCLASS()
class PROJECTFPS_API UFPSCombatAbility : public UFPSGameAbility
{
	GENERATED_BODY()

public:
	UFPSCombatAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
};
