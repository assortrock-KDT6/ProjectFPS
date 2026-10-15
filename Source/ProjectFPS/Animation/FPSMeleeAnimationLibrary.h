#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FPSMeleeAnimationLibrary.generated.h"

class UAnimMontage;
class UAnimSequence;
class UFPSAbilitySystemComponent;

// 에디터에서 근접 공격 몽타주를 구성하고 PIE 입력 검증을 보조한다.
UCLASS()
class PROJECTFPS_API UFPSMeleeAnimationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 기존 슬롯·섹션·노티파이를 교체한다. 타격 시점은 애니메이션 길이에 대한 비율이다.
	UFUNCTION(BlueprintCallable, Category = "Melee | Authoring")
	static bool ConfigureMeleeMontage(UAnimMontage* Montage, UAnimSequence* Sequence, FName SlotName,
		int32 AttackIndex, bool bFirstPerson, float HitStartFraction, float HitEndFraction);

	// 에디터 Python의 RPC 실행 제약을 피하도록 다음 게임 틱에서 PIE 입력을 실행한다.
	UFUNCTION(BlueprintCallable, Category = "Melee | Editor Tests")
	static bool QueuePIEMeleeInput(UFPSAbilitySystemComponent* AbilitySystem, bool Pressed, bool Cancel);
};
