#include "AnimGraphNode_WeaponGrip.h"

FText UAnimGraphNode_WeaponGrip::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("ProjectFPS", "WeaponGripTitle", "무기 양손 IK");
}

FText UAnimGraphNode_WeaponGrip::GetTooltipText() const
{
	return NSLOCTEXT("ProjectFPS", "WeaponGripTooltip", "최종 포즈에서 무기의 양손 그립 소켓에 손목을 맞춥니다. 리타기팅된 표시용 메시에도 적용합니다.");
}

FString UAnimGraphNode_WeaponGrip::GetNodeCategory() const
{
	return TEXT("ProjectFPS|Animation");
}

FText UAnimGraphNode_WeaponGrip::GetControllerDescription() const
{
	return GetNodeTitle(ENodeTitleType::FullTitle);
}

const FAnimNode_SkeletalControlBase* UAnimGraphNode_WeaponGrip::GetNode() const
{
	return &_Node;
}
