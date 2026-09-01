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
	using Super = FSavedMove_Character;
	bool _SavedWantsTraversal = false;
	bool _SavedWantsFinishTraversal = false;

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
	using Super = FNetworkPredictionData_Client_Character;
public:
	explicit FNetworkPredictionData_Client_FPS(const UCharacterMovementComponent& ClinetMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};

/**
 * 
 */

UCLASS()
class PROJECTFPS_API UFPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	friend class FSavedMove_FPS;
public:
	UFPSCharacterMovementComponent();

private:
	UPROPERTY(ReplicatedUsing = OnRep_TraversalData)
	FTraversalRepState _TraversalState;

private:
	bool	_WantsTraversal = false;
	uint16	_NextAuthorityActionId = 1;

private:
	UFUNCTION()
	void OnRep_TraversalData();

protected:
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	bool StartTravelsal(const FC_TraversalData& NewTraversalData);
	void StopTravelsal();

	// 서버와 Autonomous Proxy가 각각 Traversal 종료를 예측한다.
	void FinishTraversalLocally();

	void RequestTraversal();
	bool IsTraversing() const;
	bool IsTraversing(uint8 Mode) const;
	const FTraversalRepState& GetTraversalState() const;


	void RequestFinishTraversal();

	/**
	 * 현재 파쿠르 상태인지 확인한다. 
	 */

private:
	void TryStartTraversalAuthority();
	bool StartTraversalAuthority(const FTraversalCandidate& Candidate);
	void FinishTraversalAuthority();

	void PhyTraversal(float DeltaTime, int32 Interations);
	void RefreshTraversalPresentation();

	float GetAuthoritativeTimeSeconds() const;
};
