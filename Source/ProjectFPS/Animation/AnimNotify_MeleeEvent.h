#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_MeleeEvent.generated.h"

// 몸 몽타주의 타격·콤보 이벤트를 서버와 소유 클라이언트에서 각각 발생시킨다.
UCLASS(meta = (DisplayName = "Melee gameplay event"))
class PROJECTFPS_API UAnimNotify_MeleeEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 타격 시작·종료 또는 콤보 구간을 능력에 전달한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	FGameplayTag EventTag;

	// 블렌딩 중인 이전 공격의 노티파이를 구분하기 위한 콤보 단계이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	int32 AttackIndex = 0;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
