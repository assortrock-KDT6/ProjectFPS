#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "Animation/AnimNode_WeaponAim.h"
#include "AnimGraphNode_WeaponAim.generated.h"

// 런타임 보정 노드를 AnimGraph에서 배치하고 설정하기 위한 에디터 표현이다.
UCLASS()
class PROJECTFPSANIMGRAPH_API UAnimGraphNode_WeaponAim : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_WeaponAim _Node;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;

protected:
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override;
};
