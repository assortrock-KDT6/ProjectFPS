// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Parkour/MantleComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Common/GameDatas.h"
#include "GameFramework/Character.h"

UMantleComponent::UMantleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	/**
	 * 복제 프로퍼티도 RPC도 없다. 모든 트래버설 상태는 FPSCharacterMovementComponent가 복제한다.
	 * 이 컴포넌트는 로컬 후보 판정과 표현만 담당한다.
	 */
	SetIsReplicatedByDefault(false);
}

EProjectCustomMovementMode UMantleComponent::GetMode() const
{
	return EProjectCustomMovementMode::Mantle;
}

int32 UMantleComponent::GetPriority() const
{
	return 50;
}

bool UMantleComponent::BuildCandidate(const FTraversalBaseQuery& BaseQuery, FTraversalCandidate& OutCandidate) const
{
	OutCandidate = FTraversalCandidate();

	const ETraversalVariant Variant = BaseQuery._ObstacleHeight < _HighMantleThreshold ? ETraversalVariant::MantleLow : ETraversalVariant::MantleHigh;
	const FTraversalActionDefinition* Definition = FindDefinition(static_cast<uint8>(Variant));

	if (nullptr == Definition)
	{
		/**
		 * _Variant는 uint8이라 에디터에서 드롭다운이 아니라 숫자 입력으로 보인다.
		 * MantleLow=1 / MantleHigh=2 를 직접 넣지 않으면 기본값 0으로 남아 여기서 탈락한다.
		 */
		UE_LOG(LogTraversal, Verbose,
			TEXT("[Mantle] 탈락: Variant %d 에 해당하는 Definition 없음 (장애물 높이 %.1f, HighMantleThreshold %.1f). _Definitions에 _Variant=%d 행이 있는지 확인."),
			static_cast<int32>(Variant), BaseQuery._ObstacleHeight, _HighMantleThreshold, static_cast<int32>(Variant));
		return false;
	}

	if (false == IsValid(Definition->_Montage))
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Mantle] 탈락: Variant %d Definition의 _Montage가 비어 있음."), static_cast<int32>(Variant));
		return false;
	}

	if (BaseQuery._ObstacleHeight < Definition->_MinHeight
		|| BaseQuery._ObstacleHeight > Definition->_MaxHeight)
	{
		UE_LOG(LogTraversal, Verbose,
			TEXT("[Mantle] 탈락: 높이 범위 밖. 장애물 %.1f, 허용 %.1f~%.1f (둘 다 0이면 Definition의 _MinHeight/_MaxHeight 미설정)."),
			BaseQuery._ObstacleHeight, Definition->_MinHeight, Definition->_MaxHeight);
		return false;
	}

	/* Vault에는 있는 널 가드가 여기엔 없었다. */
	if (false == IsValid(_HurdleCheckComponent))
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Mantle] 탈락: HurdleCheckComponent 없음."));
		return false;
	}

	FHitResult TopFloorHit;
	if (false == _HurdleCheckComponent->CheckMantleTopFloor(BaseQuery, _TraceSettings, TopFloorHit))
	{
		UE_LOG(LogTraversal, Verbose,
			TEXT("[Mantle] 탈락: CheckMantleTopFloor 실패. 앞면에서 (캡슐반지름 + TopCheckInset)만큼 안쪽을 수직으로 훑는데, 장애물 윗면이 그만큼 깊지 않으면 허공을 찍는다."));
		return false;
	}

	if (false == _HurdleCheckComponent->CheckLandingSpace(TopFloorHit))
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Mantle] 탈락: CheckLandingSpace 실패. 올라설 자리에 캐릭터 캡슐이 안 들어간다."));
		return false;
	}

	const float Duration = GetEffectiveDuration(*Definition);

	if (Duration <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Mantle] 탈락: 유효 재생 시간 0. _PlayRate(%.2f) 또는 몽타주 RateScale 확인."), Definition->_PlayRate);
		return false;
	}

	UE_LOG(LogTraversal, Verbose, TEXT("[Mantle] 후보 생성 성공. Variant %d, 높이 %.1f, 길이 %.2fs"),
		static_cast<int32>(Variant), BaseQuery._ObstacleHeight, Duration);

	OutCandidate._Mode = GetMode();
	OutCandidate._Variant = static_cast<uint8>(Variant);
	OutCandidate._TargetLocation = TopFloorHit.ImpactPoint;
	OutCandidate._TargetRotation = BaseQuery._Direction.Rotation();
	OutCandidate._ObstaclePoint = BaseQuery._FrontHit.ImpactPoint;
	OutCandidate._ObstacleNormal = BaseQuery._FrontHit.ImpactNormal;
	OutCandidate._ObstacleComponent = BaseQuery._FrontHit.GetComponent();
	OutCandidate._Duration = Duration;

	return OutCandidate.IsValid();
}

