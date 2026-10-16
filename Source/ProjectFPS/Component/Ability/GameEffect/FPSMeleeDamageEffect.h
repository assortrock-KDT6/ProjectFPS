#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSMeleeDamageEffect.generated.h"

// 능력에서 전달한 피해량을 기존 DamageIn 속성으로 전달하는 즉시 효과이다.
UCLASS()
class PROJECTFPS_API UFPSMeleeDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFPSMeleeDamageEffect();
};
