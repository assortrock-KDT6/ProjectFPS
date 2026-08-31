// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Component/Parkour/ParkourComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UFPSCharacterMovementComponent::UFPSCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UFPSCharacterMovementComponent::StartTravelsal(const FC_TraversalData& NewTraversalData)
{
	if (nullptr == CharacterOwner)
	{
		return false;
	}

	const ENetRole LocalRole = CharacterOwner->GetLocalRole();

	const bool CanStartLocally = true == CharacterOwner->HasAuthority() || ROLE_AutonomousProxy == LocalRole;

	if (false == CanStartLocally)
	{
		return false;
	}

	if (true == IsTraversing())
	{
		return false;
	}

	if (false == NewTraversalData.IsValid())
	{
		return false;
	}

	_TraversalData = NewTraversalData;
	_TraversalElapsedTime = 0.f;
	SetMovementMode(MOVE_Custom, static_cast<uint8>(_TraversalData._Mode));
	/**
	 * 서버 자신의 표현도 즉시 갱신한다. 
	 */
	// RefreshTraversalPresentation();

	if (true == CharacterOwner->HasAuthority())
	{
		CharacterOwner->ForceNetUpdate();
	}
	return true;
}

void UFPSCharacterMovementComponent::StopTravelsal()
{
	if (nullptr == CharacterOwner 
		|| false == CharacterOwner->HasAuthority())
	{
		return;
	}

	FinishTraversalLocally();
}

void UFPSCharacterMovementComponent::FinishTraversalLocally()
{
	if (nullptr == CharacterOwner || nullptr == UpdatedComponent || false == IsTraversing())
	{
		return;
	}

	const ENetRole LocalRole = CharacterOwner->GetLocalRole();

	const bool CanFinishLocally = CharacterOwner->HasAuthority() || ROLE_AutonomousProxy == LocalRole;

	if (false == CanFinishLocally)
	{
		return;
	}

	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);

	const EMovementMode EndMovementMode = FloorResult.IsWalkableFloor() ? MOVE_Walking : MOVE_Falling;
	_TraversalElapsedTime = 0.f;

	SetMovementMode(EndMovementMode);

	if (true == CharacterOwner->HasAuthority())
	{
		_TraversalData = FC_TraversalData();											
		CharacterOwner->ForceNetUpdate();
	}
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

	if (true == IsTraversing())
	{
		return;
	}

	_WantsTraversal = true;
}

void UFPSCharacterMovementComponent::RequestFinishTraversal()
{
	if (nullptr == CharacterOwner)
	{
		return;
	}

	/**
	 * 서버의 원격 캐릭터가 서버 측 몽타주 종료 시간으로 종료를 요청하지  않도록
	 * LocallyControlled만 허용한다.
	 */

	if (false == CharacterOwner->IsLocallyControlled())
	{
		return;
	}

	if (false == IsTraversing())
	{
		return;
	}

	_WantsFinishTraversal = true;
}

bool UFPSCharacterMovementComponent::IsTraversing() const
{
	return MOVE_Custom == MovementMode && IsTraversalMode(CustomMovementMode);
}

const FC_TraversalData& UFPSCharacterMovementComponent::GetTraversalData() const
{
	return _TraversalData;
}

void UFPSCharacterMovementComponent::OnRep_TraversalData()
{
	// MovementMode와 property 복제의 도착 순서가 같다는 보장은 없다.
	RefreshTraversalPresentation();
}

FNetworkPredictionData_Client* UFPSCharacterMovementComponent::GetPredictionData_Client() const
{
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

	_WantsTraversal = ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
	_WantsFinishTraversal = ((Flags & FSavedMove_Character::FLAG_Custom_1) != 0);
}

void UFPSCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations) /* override */
{
	if (DeltaTime < MIN_TICK_TIME || false == HasValidData())
	{
		return;
	}
	
	if (false == IsTraversing())
	{
		return;
	}

	if (false == _TraversalData.IsValid())
	{
		if (nullptr != CharacterOwner && true == CharacterOwner->HasAuthority())
		{
			StopTravelsal();
		}
		return;
	}

	switch (static_cast<EProjectCustomMovementMode>(CustomMovementMode))
	{
		case EProjectCustomMovementMode::Vault:
		{
			PhysVault(DeltaTime, Iterations);
			break;
		}
		case EProjectCustomMovementMode::Mantle:
		{
			PhysMantle(DeltaTime, Iterations);
			break;
		}
		case EProjectCustomMovementMode::Hanging:
		{
			PhyHanging(DeltaTime, Iterations);
			break;
		}
		default:
		{
			if (nullptr != CharacterOwner && true == CharacterOwner->HasAuthority())
			{
				StopTravelsal();
			}
			break;
		}
	}
}

void UFPSCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	RefreshTraversalPresentation();
}

void UFPSCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFPSCharacterMovementComponent, _TraversalData);
}

void UFPSCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	/**
	 * 종료를 시작보다 먼저 처리한다. 
	 */
	if (true == _WantsFinishTraversal)
	{
		_WantsFinishTraversal = false;
		FinishTraversalLocally();
		return;
	}

	if (false == _WantsTraversal)
	{
		return;
	}

	_WantsTraversal = false;
	TryStartTraversalFromMove();
}

void UFPSCharacterMovementComponent::PhysVault(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || false == HasValidData())
	{
		return;
	}

	if (MOVE_Custom != MovementMode)
	{
		return;
	}

	if (static_cast<uint8>(EProjectCustomMovementMode::Vault) != CustomMovementMode)
	{
		return;
	}

	if (EProjectCustomMovementMode::Vault != _TraversalData._Mode)
	{
		return;
	}

	if (false == _TraversalData.IsValid())
	{
		return;
	}

	_TraversalElapsedTime += DeltaTime;

	PhysFlying(DeltaTime, Iterations);

	if( nullptr != CharacterOwner && true == CharacterOwner->HasAuthority()
		&& _TraversalElapsedTime >= _TraversalData._ExpectedDuration + _TraversalFinishGraceTime)
	{
		StopTravelsal();
	}
}

void UFPSCharacterMovementComponent::PhysMantle(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || false == HasValidData())
	{
		return;
	}

	if (MOVE_Custom != MovementMode)
	{
		return;
	}

	if (static_cast<uint8>(EProjectCustomMovementMode::Mantle) != CustomMovementMode)
	{
		return;
	}

	if (EProjectCustomMovementMode::Mantle != _TraversalData._Mode)
	{
		return;
	}

	if (false == _TraversalData.IsValid())
	{
		return;
	}

	_TraversalElapsedTime += DeltaTime;
	PhysFlying(DeltaTime, Iterations);

	if (nullptr != CharacterOwner && true == CharacterOwner->HasAuthority()
		&& _TraversalElapsedTime >= _TraversalData._ExpectedDuration + _TraversalFinishGraceTime)
	{
		StopTravelsal();
	}
}

void UFPSCharacterMovementComponent::PhyHanging(float DeltaTime, int32 Iterations)
{
	// TODO : Hanging
}

void UFPSCharacterMovementComponent::RefreshTraversalPresentation()
{
	if (nullptr == CharacterOwner)
	{
		return;
	}

	UParkourComponent* Parkour = CharacterOwner->FindComponentByClass<UParkourComponent>();

	if (false == IsValid(Parkour))
	{
		return;
	}

	if (true == IsTraversing() && _TraversalData.IsValid())
	{
		// TODO : Parkour
		Parkour->EnterVaultPresentation(_TraversalData);
	}
	else
	{
		// TODO : Parkour
		Parkour->ExitValutPresentation();
	}

}

bool UFPSCharacterMovementComponent::IsTraversalMode(uint8 Mode) const
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

void UFPSCharacterMovementComponent::TryStartTraversalFromMove()
{
	if (nullptr == CharacterOwner || true == IsTraversing())
	{
		return;
	}

	UParkourComponent* Parkour = CharacterOwner->FindComponentByClass<UParkourComponent>();

	if (false == IsValid(Parkour))
	{
		return;
	}

	Parkour->TryParkour();
}

void FSavedMove_FPS::Clear()
{
	Super::Clear();

	_SavedWantsTraversal = false;
	_SavedWantsFinishTraversal = false;
}

uint8 FSavedMove_FPS::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (true == _SavedWantsTraversal)
	{
		Result |= FLAG_Custom_0;
	}

	if (true == _SavedWantsFinishTraversal)
	{
		Result |= FLAG_Custom_1;
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

	if (NewFPSMove->_SavedWantsFinishTraversal != _SavedWantsFinishTraversal)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_FPS::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	const UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Character->GetCharacterMovement());
	if (true == IsValid(Movement))
	{
		_SavedWantsTraversal = Movement->_WantsTraversal;

		_SavedWantsFinishTraversal = Movement->_WantsFinishTraversal;
	}
}


void FSavedMove_FPS::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Character->GetCharacterMovement());
	if (true == IsValid(Movement))
	{
		// 서버 보정 후 SavedMove 재실행 시 요청을 복원한다.
		Movement->_WantsTraversal = _SavedWantsTraversal;

		Movement->_WantsFinishTraversal = _SavedWantsFinishTraversal;
	}
}

FNetworkPredictionData_Client_FPS::FNetworkPredictionData_Client_FPS(const UCharacterMovementComponent& ClinetMovement)
	:Super(ClinetMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_FPS::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_FPS());
}
