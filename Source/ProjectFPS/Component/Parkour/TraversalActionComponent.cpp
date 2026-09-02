// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Parkour/TraversalActionComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Character/CharacterPlayer.h"
#include "GameFramework/GameStateBase.h"
#include "MotionWarpingComponent.h"

DEFINE_LOG_CATEGORY(LogTraversal);

// Sets default values for this component's properties
UTraversalActionComponent::UTraversalActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

// Called when the game starts
void UTraversalActionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		return;
	}

	_HurdleCheckComponent	= Owner->FindComponentByClass<UHurdleCheckComponent>();
	_MotionWarpingComponent = Owner->FindComponentByClass<UMotionWarpingComponent>();
}

int32 UTraversalActionComponent::GetPriority() const
{
	return 0;
}

void UTraversalActionComponent::EnterPresentation(const FTraversalRepState& State)
{
	if (false == State.IsActive() || false == IsValid(_MotionWarpingComponent))
	{
		return;
	}

	/**
	 * 이미 재생을 마친 액션이면 어떤 경로로 다시 들어와도 재생하지 않는다.
	 * 종료 직후 _TraversalState가 아직 살아 있는 구간에서 위치 보정 리플레이로
	 * OnMovementModeChanged가 다시 발생하면 여기로 재진입한다.
	 */
	if (0 != State._ActionID && State._ActionID == _CompletedPresentationActionId)
	{
		UE_LOG(LogTraversal, Verbose, TEXT("[Presentation] 중복 재생 차단: ActionID %u 는 이미 재생을 마쳤다."), State._ActionID);
		return;
	}

	const FTraversalActionDefinition* Definition = FindDefinition(State._Variant);

	if (nullptr == Definition || false == IsValid(Definition->_Montage) || true == Definition->_WarapTargetName.IsNone())
	{
		return;
	}

	/* 늦게 도착한 종료 직전 상태로 끝난 루트 모션 몽타주를 0부터 되살리지 않는다. */
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = true == IsValid(World) ? World->GetGameState() : nullptr;
	if (true == IsValid(GameState)
		&& GameState->GetServerWorldTimeSeconds() >= State._ServerStartTimeSeconds + State._Duration)
	{
		return;
	}

	// 같은 Action의 서버 확인 상태가 다시 들어와도 서버 Target을 항상 반영한다.
	if (State._ActionID == _ActivePresentationActionId)
	{
		_MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(Definition->_WarapTargetName, State._TargetLocation, State._TargetRotation);
		return;
	}

	ExitPresentation();
	
	_MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(Definition->_WarapTargetName, State._TargetLocation, State._TargetRotation);

	_ActiveWarpTargetName = Definition->_WarapTargetName;
	ApplyObstacleIgnore(State);

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner))
	{
		ExitPresentation();
		return;
	}

	const float Duration = Owner->PlayAnimMontage(Definition->_Montage, Definition->_PlayRate);
	if (Duration <= 0.f)
	{
		ExitPresentation();
		return;
	}

	UE_LOG(LogTraversal, Log, TEXT("[Presentation] 몽타주 재생: %s, ActionID %u, Role %d, Duration %.3f"),
		*GetNameSafe(Definition->_Montage), State._ActionID, static_cast<int32>(Owner->GetLocalRole()), Duration);

	_ActiveMontage = Definition->_Montage;
	_ActivePresentationActionId = State._ActionID;
	_ActiveBlendOutTime = Definition->_BlendOutTime;

	if (true == Owner->IsLocallyControlled())
	{
		_TraversalController = Owner->GetController();
		if (true == _TraversalController.IsValid())
		{
			_TraversalController->SetIgnoreMoveInput(true);
		}
	}
	
}

void UTraversalActionComponent::ExitPresentation()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (true == IsValid(Owner) && true == IsValid(_ActiveMontage))
	{
		UAnimInstance* AnimInstance = (true == IsValid(Owner->GetMesh()) ? Owner->GetMesh()->GetAnimInstance() : nullptr);

		if (nullptr != AnimInstance && true == AnimInstance->Montage_IsPlaying(_ActiveMontage))
		{
			AnimInstance->Montage_Stop(_ActiveBlendOutTime, _ActiveMontage);
		}
	}

	ClearObstacleIgnore();

	if (true == IsValid(_MotionWarpingComponent) && false == _ActiveWarpTargetName.IsNone())
	{
		_MotionWarpingComponent->RemoveWarpTarget(_ActiveWarpTargetName);
	}

	if (true == _TraversalController.IsValid())
	{
		_TraversalController->SetIgnoreMoveInput(false);
	}

	_TraversalController.Reset();

	/* 재생을 마친 액션을 기억해 둔다. 같은 ActionID로는 다시 재생하지 않는다. */
	if (0 != _ActivePresentationActionId)
	{
		_CompletedPresentationActionId = _ActivePresentationActionId;
	}

	_ActiveMontage				= nullptr;

	_ActiveWarpTargetName		= NAME_None;

	_ActivePresentationActionId = 0;

	_ActiveBlendOutTime			= 0.1f;
}

const FTraversalActionDefinition* UTraversalActionComponent::FindDefinition(uint8 Variant) const
{
	return _Definitions.FindByPredicate(
		[this, Variant](const FTraversalActionDefinition& Definition)
		{
			return GetMode() == Definition._Mode && Variant == Definition._Variant;
		}
	);
}

float UTraversalActionComponent::GetEffectiveDuration(const FTraversalActionDefinition& Definition)
{
	if (false == IsValid(Definition._Montage))
	{
		return 0.f;
	}

	/* Montage_Play의 실효 재생률은 InPlayRate * Montage->RateScale 이다. */
	const float EffectiveRate = Definition._PlayRate * Definition._Montage->RateScale;

	if (EffectiveRate <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	return Definition._Montage->GetPlayLength() / EffectiveRate;
}

void UTraversalActionComponent::ApplyObstacleIgnore(const FTraversalRepState& State)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (false == IsValid(Owner) || false == IsValid(_HurdleCheckComponent))
	{
		return;
	}

	FHitResult LocalObstacleHit;
	if (false == _HurdleCheckComponent->ResolveLocalObstacle(State._ObstaclePoint, State._ObstacleNormal, LocalObstacleHit))
	{
		return;
	}

	UCapsuleComponent* CapsuleComponent = Owner->GetCapsuleComponent();
	UPrimitiveComponent* Obstacle = LocalObstacleHit.GetComponent();

	if (false == IsValid(CapsuleComponent) || false == IsValid(Obstacle))
	{
		return;
	}

	_IgnoredObstacleComponent = Obstacle;
	_AddedObstacleMoveIgnore = (false == CapsuleComponent->GetMoveIgnoreComponents().Contains(Obstacle));

	if (true == _AddedObstacleMoveIgnore)
	{
		CapsuleComponent->IgnoreComponentWhenMoving(Obstacle, true);
	}
}

void UTraversalActionComponent::ClearObstacleIgnore()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());

	if (true == _AddedObstacleMoveIgnore && true == IsValid(Owner) && true == _IgnoredObstacleComponent.IsValid())
	{
		UCapsuleComponent* CapsuleComponent = Owner->GetCapsuleComponent();
		if (nullptr != CapsuleComponent)
		{
			CapsuleComponent->IgnoreComponentWhenMoving(_IgnoredObstacleComponent.Get(), false);
		}
	}

	_IgnoredObstacleComponent.Reset();
	_AddedObstacleMoveIgnore = false;
}
