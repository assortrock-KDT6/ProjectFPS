// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Common/GameDatas.h"
#include "Common/PacketDatas.h"
#include "FPSCharacterMovementComponent.generated.h"

class FSavedMove_FPS final : public FSavedMove_Character
{
public:
	// using Super = FSavedMove_Character 를 안해주면 
	// 내부적으로 Super가 무엇인지 확실치 않아서 작동을 안함.
	// 그래서 using Super = FSavedMove_Character를 해주면 명시적으로 표기하는거기때문에
	// 제대로 작동함.
	// 하지만 이 클래스에선 명시성을 보여주기 위해 FSavedMove_Character:: 로 사용할 예정.
	bool _SavedWantsTraversal = false;

public:
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData);
	virtual void PrepMoveFor(ACharacter* Character) override;
};

class FNetworkPredictionData_Client_FPS final : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_FPS(const UCharacterMovementComponent& ClinetMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};

class UTraversalActionComponent;

UCLASS()
class PROJECTFPS_API UFPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	friend class FSavedMove_FPS;
public:
	UFPSCharacterMovementComponent();

private:
	UPROPERTY(ReplicatedUsing = OnRep_TraversalState)
	FTraversalRepState _TraversalState;

protected:
	/**
	 * 서버가 상태를 소유 클라이언트에 먼저 전달하기 위해 시작을 예약하는 최대 시간(초).
	 * 실제 예약값은 예상 편도 지연이며, 호스트가 직접 개시하면 지연하지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Traversal", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float _MaxTraversalStartDelay = 0.25f;

	/* AnimNotify가 누락되거나 서버에서 애니메이션 Tick이 멈춘 경우에만 사용하는 종료 안전장치. */
	UPROPERTY(EditDefaultsOnly, Category = "Traversal", meta = (ClampMin = "0.0"))
	float _TraversalEndWatchdogDelay = 0.5f;

private:
	bool	_WantsTraversal = false;
	uint16	_NextAuthorityActionId = 1;
	uint16	_CompletedAutonomousActionId = 0;

	/* 트래버설 중 bOrientRotationToMovement를 끄고 되돌리기 위한 캐시. */
	bool	_CachedOrientRotationToMovement = false;
	bool	_TraversalRotationOverridden = false;

	TWeakObjectPtr<UTraversalActionComponent> _ActivePresentationComponent;
private:
	UFUNCTION()
	void OnRep_TraversalState();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	void  RequestTraversal();
	bool  IsTraversing() const;
	bool  IsTraversing(uint8 Mode) const;
	const FTraversalRepState& GetTraversalState() const;
	void  NotifyTraversalEnded();

	/* 트래버설 예약에 쓰는 서버 시각. 소유 클라이언트에서는 예상 편도 지연을 보상한다. */
	float GetServerTimeSeconds() const;

private:
	bool  TryBuildTraversalCandidate(FTraversalCandidate& OutCandidate) const;
	void  TryStartTraversalAuthority();
	bool  StartTraversalAuthority(const FTraversalCandidate& Candidate);
	void  FinishTraversalAuthority();
	UTraversalActionComponent* FindTraversalActionComponent(EProjectCustomMovementMode Mode) const;
		  
	void  PhyTraversal(float DeltaTime, int32 Iterations);
	void  RefreshTraversalPresentation();

	/* 예약 시각에 도달한 트래버설을 Authority/AutonomousProxy에서 시작한다. */
	bool  TryEnterPendingTraversal();
	void  EnterTraversalMovementMode(EProjectCustomMovementMode Mode);
	void  ExitTraversalMovementMode();

	float ComputeTraversalStartDelay(float& OutEstimatedOneWaySeconds) const;
	void  UpdateTraversalRotationOverride();
};
