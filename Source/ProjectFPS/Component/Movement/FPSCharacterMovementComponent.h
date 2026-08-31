// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	static inline const float _TraversalFinishGraceTime = 0.75f;

	float _TraversalElapsedTime = 0.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_TraversalData)
	FC_TraversalData _TraversalData;

private:
	bool _WantsTraversal = false;
	bool _WantsFinishTraversal = false;
private:
	UFUNCTION()
	void OnRep_TraversalData();
public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
public:
	bool StartTravelsal(const FC_TraversalData& NewTraversalData);
	void StopTravelsal();

	// 서버와 Autonomous Proxy가 각각 Traversal 종료를 예측한다.
	void FinishTraversalLocally();
	void RequestTraversal();
	void RequestFinishTraversal();

	/**
	 * 현재 파쿠르 상태인지 확인한다. 
	 */
	bool IsTraversing() const;
	const FC_TraversalData& GetTraversalData() const;

private:
	void PhysVault(float DeltaTime, int32 Iterations);
	void PhysMantle(float DeltaTime, int32 Iterations);
	void PhyHanging(float DeltaTime, int32 Iterations);
	void RefreshTraversalPresentation();
	bool IsTraversalMode(uint8 Mode) const;

	void TryStartTraversalFromMove();
};
