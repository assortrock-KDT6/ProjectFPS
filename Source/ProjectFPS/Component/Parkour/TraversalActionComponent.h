// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/GameDefines.h"
#include "Common/GameDatas.h"
#include "Common/PacketDatas.h"
#include "Components/ActorComponent.h"
#include "TraversalActionComponent.generated.h"

/**
 * 공통 추상 기반 클래스
 */

class UHurdleCheckComponent;
class UMotionWarpingComponent;

/**
 * 트래버설 후보 판정 진단용.
 * 콘솔에서 "Log LogTraversal Verbose" 로 켜면 어떤 검사에서 탈락했는지 전부 찍힌다.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogTraversal, Log, All);

UCLASS(Abstract)
class PROJECTFPS_API UTraversalActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTraversalActionComponent();

protected:
	// Transient : 디스크로부터 데이터를 로딩하는 것을 방지할 때 사용
	UPROPERTY(Transient)
	TObjectPtr<UHurdleCheckComponent> _HurdleCheckComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMotionWarpingComponent> _MotionWarpingComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Definitions")
	TArray<FTraversalActionDefinition> _Definitions;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> _ActiveMontage;

private:
	TWeakObjectPtr<UPrimitiveComponent> _IgnoredObstacleComponent;
	TWeakObjectPtr<AController>			_TraversalController;
	
	FName	_ActiveWarpTargetName = NAME_None;

	/* 현재 재생 중인 액션. ExitPresentation에서 0으로 초기화된다. */
	uint16	_ActivePresentationActionId = 0;

	/**
	 * 이미 재생을 마친 액션.
	 * _ActivePresentationActionId는 Exit할 때 지워지므로 "이 액션을 이미 재생했다"는
	 * 기억이 사라진다. _TraversalState는 서버의 종료 복제가 도착할 때까지 살아 있어서,
	 * 그 사이 위치 보정/무브 리플레이로 MovementMode가 다시 MOVE_Custom이 되면
	 * 같은 _ActionID로 몽타주가 두 번 재생된다. 이 값이 그걸 막는다.
	 */
	uint16	_CompletedPresentationActionId = 0;
	float	_ActiveBlendOutTime = 0.1f;
	bool	_AddedObstacleMoveIgnore = false;

public:
	virtual EProjectCustomMovementMode GetMode() const PURE_VIRTUAL(UTraversalActionComponent::GetMode, return EProjectCustomMovementMode::None;);
	virtual int32	GetPriority() const;
	virtual bool	BuildCandidate(const FTraversalBaseQuery& BaseQuery, FTraversalCandidate& OutCandidate) const PURE_VIRTUAL(UTraversalActionComponent::BuildCandidate, return false;);
public:
	void EnterPresentation(const FTraversalRepState& State);
	void ExitPresentation();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
	const	FTraversalActionDefinition* FindDefinition(uint8 Variant) const;

	/**
	 * 몽타주가 실제로 재생되는 시간(초).
	 * Montage_Play의 실효 재생률은 _PlayRate * Montage->RateScale 이므로 둘 다 반영해야
	 * 서버의 트래버설 종료 시각과 애니메이션 길이가 일치한다.
	 */
	static	float GetEffectiveDuration(const FTraversalActionDefinition& Definition);

	void	ApplyObstacleIgnore(const FTraversalRepState& State);
	void	ClearObstacleIgnore();

};
