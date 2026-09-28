#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSMatchCombatBlockEffect.generated.h"

/** 매치 대기/카운트다운/종료 동안 서버가 유지하는 전투 제한 효과. */
UCLASS()
class PROJECTFPS_API UFPSMatchCombatBlockEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFPSMatchCombatBlockEffect();
};
