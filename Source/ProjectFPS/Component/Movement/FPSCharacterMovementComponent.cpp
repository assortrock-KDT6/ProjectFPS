// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Component/Parkour/TraversalActionComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

UFPSCharacterMovementComponent::UFPSCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFPSCharacterMovementComponent::RequestTraversal()
{
	if (nullptr == CharacterOwner)
	{
		return;
	}

	if (false == CharacterOwner->IsLocallyControlled())
	{
		return;
	}

	/**
	 * 예약 대기 구간(_TraversalState는 활성이지만 아직 시작 시각 전)에도 재요청을 막는다.
	 * IsTraversing()만 보면 이 구간에서 두 번째 요청이 서버로 나간다.
	 */
	if (true == IsTraversing() || true == _TraversalState.IsActive())
	{
		return;
	}

	_WantsTraversal = true;
}

bool UFPSCharacterMovementComponent::IsTraversing() const
{
	return MOVE_Custom == MovementMode && IsTraversing(CustomMovementMode);
}

const FTraversalRepState& UFPSCharacterMovementComponent::GetTraversalState() const
{
	return _TraversalState;
}

void UFPSCharacterMovementComponent::NotifyTraversalEnded()
{
	if (nullptr == CharacterOwner || false == IsTraversing())
	{
		return;
	}

	const ENetRole LocalRole = CharacterOwner->GetLocalRole();
	if (ROLE_Authority == LocalRole)
	{
		FinishTraversalAuthority();
		return;
	}

	if (ROLE_AutonomousProxy == LocalRole)
	{
		_CompletedAutonomousActionId = _TraversalState._ActionID;
		ExitTraversalMovementMode();
	}
}

void UFPSCharacterMovementComponent::OnRep_TraversalState()
{
	const bool IsAutonomousProxy = nullptr != CharacterOwner
		&& ROLE_AutonomousProxy == CharacterOwner->GetLocalRole();

	if (false == _TraversalState.IsActive())
	{
		_CompletedAutonomousActionId = 0;

		/**
		 * ReplicatedMovementMode는 SimulatedProxy에만 오므로 소유 클라이언트는
		 * 복제 상태가 비워졌을 때 직접 기본 이동 모드로 돌아간다.
		 */
		if (true == IsAutonomousProxy)
		{
			ExitTraversalMovementMode();
		}
	}
	else
	{
		/* SimulatedProxy는 엔진이 복제한 MovementMode를 기다린다. */
		TryEnterPendingTraversal();
	}

	RefreshTraversalPresentation();
}

void UFPSCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	/* Authority/AutonomousProxy만 예약 시각을 감시한다. 역할 검사는 함수 내부에서 한다. */
	TryEnterPendingTraversal();

	/**
	 * 서버가 긴 프레임 정지 등으로 예약 구간 전체를 놓쳤다면 상태만 정리한다.
	 */
	if (nullptr != CharacterOwner && true == CharacterOwner->HasAuthority()
		&& true == _TraversalState.IsActive() && false == IsTraversing()
		&& GetServerTimeSeconds() > _TraversalState._ServerStartTimeSeconds + _TraversalState._Duration)
	{
		_TraversalState = FTraversalRepState();
		CharacterOwner->ForceNetUpdate();
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/**
	 * 서버 보정은 Super::TickComponent 안에서 SavedMove를 과거부터 재실행한다.
	 * 그 과정의 임시 MovementMode가 아니라 리플레이가 끝난 최종 모드만 표현에 반영한다.
	 */
	if (nullptr != CharacterOwner && false == CharacterOwner->bClientUpdating)
	{
		UpdateTraversalRotationOverride();
		RefreshTraversalPresentation();
	}
}

FNetworkPredictionData_Client* UFPSCharacterMovementComponent::GetPredictionData_Client() const
{
	check(nullptr != PawnOwner);

	if (nullptr == ClientPredictionData)
	{
		UFPSCharacterMovementComponent* MutableThis = const_cast<UFPSCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_FPS(*this);
	}
	return ClientPredictionData;
}

void UFPSCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// |AND 연산						|
	// |----------------------------|
	// |경우 1	|경우 2				|
	// |0 1 0 0 | 0 0 0 1 			|
	// |1 0 0 0 | 0 0 0 1			|
	// |0 0 0 0 | 0 0 0 1			|
	// |----------------------------|
	// |값 0	    | 값 1				|
	// |----------------------------|
	// |!= 0 연산 (0과 같지 않다.)	|
	// |----------------------------|
	// |false   | true				|

	// 켜진 비트의 자리가 FLAG_Custom_0 와 동일할 때 true
	_WantsTraversal = ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
}

void UFPSCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations) /* override */
{
	if (DeltaTime < MIN_TICK_TIME || false == HasValidData())
	{
		return;
	}

	switch (static_cast<EProjectCustomMovementMode>(CustomMovementMode))
	{
		case EProjectCustomMovementMode::Vault:
		case EProjectCustomMovementMode::Mantle:
		{
			PhyTraversal(DeltaTime, Iterations);
			return;
		}
		case EProjectCustomMovementMode::Hanging:
		{
			// TODO : Hanging 움직임.
			return;
		}
		default:
		{
			Super::PhysCustom(DeltaTime, Iterations);
			return;
		}
	}
}

void UFPSCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	/**
	 * ClientUpdatePositionAfterServerUpdate는 SavedMove마다 SetMovementMode를 수행한다.
	 * 여기서 몽타주/Warp Target을 끊으면 다음 SavedMove의 루트모션이 다른 조건으로
	 * 재실행되어 이동 중 위치 보정이 반복된다. TickComponent 끝에서 최종 상태를 갱신한다.
	 */
	if (nullptr != CharacterOwner && true == CharacterOwner->bClientUpdating)
	{
		return;
	}

	UpdateTraversalRotationOverride();
	RefreshTraversalPresentation();
}

void UFPSCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSCharacterMovementComponent, _TraversalState);
}

void UFPSCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (false == _WantsTraversal)
	{
		return;
	}

	_WantsTraversal = false;

	if (nullptr != CharacterOwner && true == CharacterOwner->HasAuthority())
	{
		TryStartTraversalAuthority();
	}
}

bool UFPSCharacterMovementComponent::TryBuildTraversalCandidate(FTraversalCandidate& OutCandidate) const
{
	if (false == IsValid(CharacterOwner) || false == CharacterOwner->HasAuthority())
	{
		return false;
	}

	UHurdleCheckComponent* HurdleCheckComponent = CharacterOwner->FindComponentByClass<UHurdleCheckComponent>();
	if (false == IsValid(HurdleCheckComponent))
	{
		return false;
	}

	FTraversalBaseQuery BaseQuery;
	if (false == HurdleCheckComponent->BuildBaseQuery(BaseQuery))
	{
		UE_LOG(LogTraversal, Verbose,
			TEXT("[Traversal] 탈락: BuildBaseQuery 실패. 앞면/윗면 트레이스가 안 잡혔거나 서로 다른 장애물이다. Vault/Mantle 둘 다 시도조차 못 한다."));
		return false;
	}

	TArray<UTraversalActionComponent*> Actions;
	CharacterOwner->GetComponents(Actions);

	Actions.RemoveAll(
		[](const UTraversalActionComponent* Action)
		{
			return false == IsValid(Action);
		}
	);

	Actions.Sort(
		[](const UTraversalActionComponent& Left, const UTraversalActionComponent& Right)
		{
			return Left.GetPriority() > Right.GetPriority();
		}
	);

	UE_LOG(LogTraversal, Verbose, TEXT("[Traversal] BaseQuery 성공. 장애물 높이 %.1f, 후보 컴포넌트 %d개 (우선순위 내림차순)."),
		BaseQuery._ObstacleHeight, Actions.Num());

	for (const UTraversalActionComponent* Action : Actions)
	{
		if (true == IsValid(Action) && true == Action->BuildCandidate(BaseQuery, OutCandidate))
		{
			UE_LOG(LogTraversal, Verbose, TEXT("[Traversal] 선택됨: %s (우선순위 %d). 이 뒤의 컴포넌트는 시도하지 않는다."),
				*Action->GetName(), Action->GetPriority());
			return true;
		}
	}

	UE_LOG(LogTraversal, Verbose, TEXT("[Traversal] 모든 후보 실패. 파쿠르 입력이 무시된다."));
	return false;
}

void UFPSCharacterMovementComponent::TryStartTraversalAuthority()
{
	if (false == IsValid(CharacterOwner))
	{
		return;
	}

	FTraversalCandidate Candidate;
	if (true == TryBuildTraversalCandidate(Candidate))
	{
		StartTraversalAuthority(Candidate);
	}
}

bool UFPSCharacterMovementComponent::StartTraversalAuthority(const FTraversalCandidate& Candidate)
{
	if (nullptr == CharacterOwner
		|| false == CharacterOwner->HasAuthority()
		|| true == IsTraversing() || true == _TraversalState.IsActive() || false == Candidate.IsValid())
	{
		return false;
	}

	_TraversalState = FTraversalRepState();
	_TraversalState._Mode = Candidate._Mode;
	_TraversalState._Variant = Candidate._Variant;
	_TraversalState._ActionID = _NextAuthorityActionId++;
	_TraversalState._TargetLocation = Candidate._TargetLocation;
	_TraversalState._TargetRotation = Candidate._TargetRotation;
	_TraversalState._ObstaclePoint = Candidate._ObstaclePoint;
	_TraversalState._ObstacleNormal = Candidate._ObstacleNormal;
	/**
	 * 즉시 시작하지 않고 시작 시각을 약간 미래로 예약한다.
	 * 상태를 먼저 복제해 두면 서버와 소유 클라이언트가 같은 서버 시각에 동시에 시작할 수 있고,
	 * 그러면 Montage_SetPosition으로 루트모션을 건너뛸 필요가 없어진다.
	 */
	float EstimatedOneWaySeconds = 0.f;
	const float StartDelay = ComputeTraversalStartDelay(EstimatedOneWaySeconds);
	_TraversalState._ServerStartTimeSeconds = GetServerTimeSeconds() + StartDelay;
	_TraversalState._EstimatedOneWaySeconds = EstimatedOneWaySeconds;
	_TraversalState._Duration = Candidate._Duration;

	if (0 == _NextAuthorityActionId)
	{
		_NextAuthorityActionId = 1;
	}

	CharacterOwner->ForceNetUpdate();

	/* 지연이 0이면(호스트 자신이 개시) 이번 프레임에 곧바로 진입한다. */
	TryEnterPendingTraversal();
	return true;
}

void UFPSCharacterMovementComponent::FinishTraversalAuthority()
{
	if (nullptr == CharacterOwner 
		|| false == CharacterOwner->HasAuthority() || false == IsTraversing())
	{
		return;
	}

	_TraversalState = FTraversalRepState();

	ExitTraversalMovementMode();
	CharacterOwner->ForceNetUpdate();
}

UTraversalActionComponent* UFPSCharacterMovementComponent::FindTraversalActionComponent(EProjectCustomMovementMode Mode) const
{
	if (false == IsValid(CharacterOwner))
	{
		return nullptr;
	}

	TArray<UTraversalActionComponent*> Actions;
	CharacterOwner->GetComponents(Actions);

	for (UTraversalActionComponent* Action : Actions)
	{
		if (true == IsValid(Action) && Mode == Action->GetMode())
		{
			return Action;
		}
	}

	return nullptr;
}

void UFPSCharacterMovementComponent::PhyTraversal(float DeltaTime, int32 Iterations)
{
	PhysFlying(DeltaTime, Iterations);

	if (nullptr == CharacterOwner)
	{
		return;
	}

	/* 정상 종료는 AnimNotify가 담당하고, 이 시각은 Notify 누락 시에만 사용하는 watchdog이다. */
	const float EndTime = _TraversalState._ServerStartTimeSeconds + _TraversalState._Duration + _TraversalEndWatchdogDelay;

	if (GetServerTimeSeconds() < EndTime)
	{
		return;
	}

	const ENetRole LocalRole = CharacterOwner->GetLocalRole();

	if (ROLE_Authority == LocalRole)
	{
		FinishTraversalAuthority();
		return;
	}

	/**
	 * 소유 클라이언트는 ReplicatedMovementMode 대상이 아니므로 예약 종료 시 직접 이탈한다.
	 * SimulatedProxy는 엔진의 ReplicatedMovementMode/RepRootMotion 경로에 맡긴다.
	 */
	if (ROLE_AutonomousProxy == LocalRole)
	{
		_CompletedAutonomousActionId = _TraversalState._ActionID;
		ExitTraversalMovementMode();
	}
}

void UFPSCharacterMovementComponent::RefreshTraversalPresentation()
{
	if (nullptr == CharacterOwner)
	{
		return;
	}

	/* SavedMove 리플레이 중의 임시 이동 모드는 표현 수명에 반영하지 않는다. */
	if (true == CharacterOwner->bClientUpdating)
	{
		return;
	}

	/**
	 * 소유 클라이언트가 이 ActionID의 예약 종료를 이미 처리한 뒤에도 서버의 마지막
	 * MOVE_Custom 보정이 늦게 도착할 수 있다. 이때 MovementMode만 다시 Custom이 되어도
	 * 끝난 몽타주를 0부터 재생하지 않도록 표현 계층에서도 완료 ActionID를 차단한다.
	 */
	const bool IsCompletedAutonomousAction = ROLE_AutonomousProxy == CharacterOwner->GetLocalRole()
		&& _TraversalState._ActionID == _CompletedAutonomousActionId;
	const bool HasReachedWatchdogEnd = _TraversalState.IsActive()
		&& GetServerTimeSeconds() >= _TraversalState._ServerStartTimeSeconds
			+ _TraversalState._Duration + _TraversalEndWatchdogDelay;

	/* 상태가 끝났거나 Notify 종료가 처리됐거나 watchdog에 도달한 경우에만 표현을 종료한다. */
	if (false == _TraversalState.IsActive() || true == IsCompletedAutonomousAction || true == HasReachedWatchdogEnd)
	{
		if (true == _ActivePresentationComponent.IsValid())
		{
			_ActivePresentationComponent->ExitPresentation();
		}
		_ActivePresentationComponent.Reset();

		/**
		 * return이 없으면 종료 처리 직후 아래로 흘러내려 EnterPresentation이 다시 불린다.
		 * OnRep_TraversalState와 OnMovementModeChanged는 도착 순서가 보장되지 않으므로
		 * 진입은 반드시 "MovementMode 진입 -> 표현 갱신" 순서로만 일어나야 한다.
		 */
		return;
	}

	/**
	 * 활성 Action의 중간에 서버 보정으로 MovementMode가 잠시 Walking/Falling이 되어도
	 * 몽타주와 Warp Target을 유지한다. 여기서 Exit 후 다시 Enter하면 같은 ActionID가
	 * 시간차를 두고 0초부터 두 번째 재생된다.
	 */
	if (false == IsTraversing())
	{
		return;
	}

	UTraversalActionComponent* RequestedComponent = FindTraversalActionComponent(_TraversalState._Mode);

	if (false == IsValid(RequestedComponent))
	{
		return;
	}

	if (true == _ActivePresentationComponent.IsValid() && RequestedComponent != _ActivePresentationComponent.Get())
	{
		_ActivePresentationComponent->ExitPresentation();
	}

	_ActivePresentationComponent = RequestedComponent;
	RequestedComponent->EnterPresentation(_TraversalState);
}

float UFPSCharacterMovementComponent::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (false == IsValid(World))
	{
		return 0.f;
	}

	const AGameStateBase* GameState = World->GetGameState();

	if (true == IsValid(GameState))
	{
		float ServerTimeSeconds = GameState->GetServerWorldTimeSeconds();

		/**
		 * GameState의 클라이언트 서버 시각은 전송 시간만큼 뒤에 있다. 실시간 Ping을 여기서
		 * 다시 읽으면 서버가 예약할 때의 값과 달라져 액션마다 시작 위상이 튄다. 서버가 해당
		 * 액션을 만들 때 확정해 복제한 편도 지연 추정값만 사용한다.
		 */
		if (nullptr != CharacterOwner && ROLE_AutonomousProxy == CharacterOwner->GetLocalRole())
		{
			ServerTimeSeconds += FMath::Max(0.f, _TraversalState._EstimatedOneWaySeconds);
		}

		return ServerTimeSeconds;
	}

	/**
	 * GameState가 아직 복제되지 않은 구간.
	 * 서버는 로컬 시간이 곧 서버 시간이다.
	 * 클라에서 로컬 시간을 쓰면 경과 시간이 엉뚱하게 커져 몽타주가 통째로 스킵되므로,
	 * 예약 시각을 그대로 돌려 경과 시간이 0이 되게 한다.
	 */
	if (nullptr != CharacterOwner && true == CharacterOwner->HasAuthority())
	{
		return World->GetTimeSeconds();
	}

	return _TraversalState._ServerStartTimeSeconds;
}

bool UFPSCharacterMovementComponent::TryEnterPendingTraversal()
{
	if (nullptr == CharacterOwner || false == _TraversalState.IsActive() || true == IsTraversing()
		|| _TraversalState._ActionID == _CompletedAutonomousActionId)
	{
		return false;
	}

	const ENetRole LocalRole = CharacterOwner->GetLocalRole();
	if (ROLE_Authority != LocalRole && ROLE_AutonomousProxy != LocalRole)
	{
		return false;
	}

	const float ServerTimeSeconds = GetServerTimeSeconds();
	const float EndTimeSeconds = _TraversalState._ServerStartTimeSeconds + _TraversalState._Duration;

	/* 만료된 상태로 재진입하면 상태-clear가 올 때까지 Custom/기본 모드를 반복하게 된다. */
	if (ServerTimeSeconds < _TraversalState._ServerStartTimeSeconds || ServerTimeSeconds >= EndTimeSeconds)
	{
		return false;
	}

	EnterTraversalMovementMode(_TraversalState._Mode);
	return true;
}

void UFPSCharacterMovementComponent::EnterTraversalMovementMode(EProjectCustomMovementMode Mode)
{
	if (nullptr == CharacterOwner || ROLE_SimulatedProxy == CharacterOwner->GetLocalRole()
		|| false == IsTraversing(static_cast<uint8>(Mode)))
	{
		return;
	}

	SetMovementMode(MOVE_Custom, static_cast<uint8>(Mode));
}

void UFPSCharacterMovementComponent::ExitTraversalMovementMode()
{
	if (nullptr == CharacterOwner || ROLE_SimulatedProxy == CharacterOwner->GetLocalRole()
		|| nullptr == UpdatedComponent || false == IsTraversing())
	{
		return;
	}

	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);

	SetMovementMode(true == FloorResult.IsWalkableFloor() ? MOVE_Walking : MOVE_Falling);
}

float UFPSCharacterMovementComponent::ComputeTraversalStartDelay(float& OutEstimatedOneWaySeconds) const
{
	OutEstimatedOneWaySeconds = 0.f;

	if (false == IsValid(CharacterOwner))
	{
		return 0.f;
	}

	/**
	 * 상태를 먼저 받아야 하는 것은 자기 이동을 시뮬레이션하는 AutonomousProxy다.
	 * SimulatedProxy는 서버의 ReplicatedMovementMode와 RepRootMotion을 따라간다.
	 * 따라서 개시자가 호스트 자신이면 지연을 두지 않는다.
	 */
	if (ROLE_AutonomousProxy != CharacterOwner->GetRemoteRole())
	{
		return 0.f;
	}

	const APlayerState* PlayerState = CharacterOwner->GetPlayerState();
	if (false == IsValid(PlayerState))
	{
		return 0.f;
	}

	/* 편도 전송 시간에 약 두 프레임의 복제/스케줄링 여유를 더한다. */
	constexpr float ReplicationSafetyMarginSeconds = 1.f / 30.f;
	const float OneWaySeconds = PlayerState->GetPingInMilliseconds() * 0.5f * 0.001f;
	const float StartDelay = FMath::Clamp(OneWaySeconds + ReplicationSafetyMarginSeconds, 0.f, _MaxTraversalStartDelay);

	/* Clamp된 예약값과 동일한 기준을 클라이언트가 사용하도록 실제 반영된 편도분만 저장한다. */
	OutEstimatedOneWaySeconds = FMath::Max(0.f, StartDelay - ReplicationSafetyMarginSeconds);
	return StartDelay;
}

void UFPSCharacterMovementComponent::UpdateTraversalRotationOverride()
{
	const bool ShouldOverride = IsTraversing();

	if (true == ShouldOverride && false == _TraversalRotationOverridden)
	{
		/* Motion Warping이 맞춘 회전을 bOrientRotationToMovement가 매 프레임 덮어쓰는 것을 막는다. */
		_CachedOrientRotationToMovement = bOrientRotationToMovement;
		bOrientRotationToMovement = false;
		_TraversalRotationOverridden = true;
		return;
	}

	if (false == ShouldOverride && true == _TraversalRotationOverridden)
	{
		bOrientRotationToMovement = _CachedOrientRotationToMovement;
		_TraversalRotationOverridden = false;
	}
}

bool UFPSCharacterMovementComponent::IsTraversing(uint8 Mode) const
{
	switch (static_cast<EProjectCustomMovementMode>(Mode))
	{
		case EProjectCustomMovementMode::Vault:
		case EProjectCustomMovementMode::Mantle:
		case EProjectCustomMovementMode::Hanging:
		{
			return true;
		}
		case EProjectCustomMovementMode::None:
		default:
		{
			return false;
		}
	}
}

void FSavedMove_FPS::Clear()
{
	FSavedMove_Character::Clear();

	_SavedWantsTraversal = false;
}

uint8 FSavedMove_FPS::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (true == _SavedWantsTraversal)
	{
		Result |= FLAG_Custom_0;
	}

	return Result;
}

bool FSavedMove_FPS::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_FPS* NewFPSMove = static_cast<const FSavedMove_FPS*>(NewMove.Get());

	/**
	 * 파쿠르 요청 Move와 일반 Move가 합쳐지면 버튼 입력 프레임을 잃을 수 있다. 
	 */
	if (NewFPSMove->_SavedWantsTraversal != _SavedWantsTraversal)
	{
		return false;
	}

	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_FPS::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	const UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Character->GetCharacterMovement());
	
	// 둘 다 참이 아니면 false
	_SavedWantsTraversal = true == IsValid(Movement) && true == Movement->_WantsTraversal;
}


void FSavedMove_FPS::PrepMoveFor(ACharacter* Character)
{
	FSavedMove_Character::PrepMoveFor(Character);

	UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Character->GetCharacterMovement());
	
	if (true == IsValid(Movement))
	{
		// 서버 보정 후 SavedMove 재실행 시 요청을 복원한다.
		Movement->_WantsTraversal = _SavedWantsTraversal;
	}
}

FNetworkPredictionData_Client_FPS::FNetworkPredictionData_Client_FPS(const UCharacterMovementComponent& ClinetMovement)
	:FNetworkPredictionData_Client_Character(ClinetMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_FPS::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_FPS());
}
