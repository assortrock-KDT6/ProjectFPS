#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FPSWeaponAimPostProcessInstance.generated.h"

// 이전 후처리 블루프린트의 클래스 참조를 읽기 위한 호환용 클래스다.
// 총구 보정은 후처리 AnimGraph의 WeaponAim 노드에서만 수행한다.
UCLASS(Transient, Blueprintable)
class PROJECTFPS_API UFPSWeaponAimPostProcessInstance : public UAnimInstance
{
	GENERATED_BODY()

};
