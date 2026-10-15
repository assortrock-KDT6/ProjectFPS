#include "AnimGraphNode_WeaponAim.h"

FText UAnimGraphNode_WeaponAim::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("ProjectFPS", "WeaponAimTitle", "총구 조준 보정");
}

FText UAnimGraphNode_WeaponAim::GetTooltipText() const
{
	return NSLOCTEXT("ProjectFPS", "WeaponAimTooltip", "기존 포즈와 IK 다음에 배치합니다. 실제 부착 소켓을 기준으로 상체와 양손을 함께 회전하여 총구를 조준점에 맞춥니다.");
}

FString UAnimGraphNode_WeaponAim::GetNodeCategory() const
{
	return TEXT("ProjectFPS|Animation");
}

FText UAnimGraphNode_WeaponAim::GetControllerDescription() const
{
	return GetNodeTitle(ENodeTitleType::FullTitle);
}

const FAnimNode_SkeletalControlBase* UAnimGraphNode_WeaponAim::GetNode() const
{
	return &_Node;
}
