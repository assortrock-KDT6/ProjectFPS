// Fill out your copyright notice in the Description page of Project Settings.

#include "VaultComponent.h"
#include "Common/GameDatas.h"
#include "HurdleCheckComponent.h"

// Sets default values for this component's properties
UVaultComponent::UVaultComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	/**
	 * 복제 프로퍼티도 RPC도 없다. 모든 트래버설 상태는 FPSCharacterMovementComponent가 복제한다.
	 * 이 컴포넌트는 로컬 후보 판정과 표현만 담당한다.
	 */
	SetIsReplicatedByDefault(false);
}

EProjectCustomMovementMode UVaultComponent::GetMode() const
{
	return EProjectCustomMovementMode::Vault;
}

int32 UVaultComponent::GetPriority() const
{
	return 100;
}

bool UVaultComponent::BuildCandidate(const FTraversalBaseQuery& BaseQuery, FTraversalCandidate& OutCandidate) const
{
	OutCandidate = FTraversalCandidate();

	const FTraversalActionDefinition* Definition = FindDefinition(static_cast<uint8>(ETraversalVariant::Default));

	if (nullptr == Definition || false == IsValid(Definition->_Montage))
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Vault] 탈락: Variant 0(Default) Definition 또는 몽타주 없음."));
		return false;
	}

	if (BaseQuery._ObstacleHeight < Definition->_MinHeight || BaseQuery._ObstacleHeight > Definition->_MaxHeight)
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Vault] 탈락: 높이 범위 밖. 장애물 %.1f, 허용 %.1f~%.1f"),
			BaseQuery._ObstacleHeight, Definition->_MinHeight, Definition->_MaxHeight);
		return false;
	}

	FHitResult BackHit;
	float ObstacleDepth = 0.f;

	if (false == IsValid(_HurdleCheckComponent))
	{
		return false;
	}

	if (false == _HurdleCheckComponent->CheckVaultBackBlock(BaseQuery, _TraceSettings, BackHit, ObstacleDepth))
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Vault] 탈락: 뒷면 검출 실패 (넘어갈 수 없는 두께/형상)."));
		return false;
	}

	if (ObstacleDepth > Definition->_MaxDepth)
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Vault] 탈락: 너무 두꺼움. 깊이 %.1f > _MaxDepth %.1f"), ObstacleDepth, Definition->_MaxDepth);
		return false;
	}

	FHitResult LandingHit;

	if (false == _HurdleCheckComponent->CheckVaultLandingFloor(BaseQuery, BackHit, _TraceSettings, LandingHit))
	{
		return false;
	}

	if (false == _HurdleCheckComponent->CheckLandingSpace(LandingHit))
	{
		return false;
	}

	const float Duration = GetEffectiveDuration(*Definition);

	if (Duration <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutCandidate._Mode = EProjectCustomMovementMode::Vault;
	OutCandidate._Variant = Definition->_Variant;
	OutCandidate._TargetLocation = LandingHit.ImpactPoint;
	OutCandidate._TargetRotation = BaseQuery._Direction.Rotation();
	OutCandidate._ObstaclePoint = BaseQuery._FrontHit.ImpactPoint;
	OutCandidate._ObstacleNormal = BaseQuery._FrontHit.ImpactNormal;
	OutCandidate._ObstacleComponent = BaseQuery._FrontHit.GetComponent();
	OutCandidate._Duration = Duration;
	
	return OutCandidate.IsValid();
}