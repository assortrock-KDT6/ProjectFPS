// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/GameDefines.h"
#include "Components/ActorComponent.h"
#include "TraversalActionComponent.generated.h"

/**
 * 공통 추상 기반 클래스
 */

UCLASS(Abstract)
class PROJECTFPS_API UTraversalActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTraversalActionComponent();

public:
	virtual EProjectCustomMovementMode GetMode() const PURE_VIRTUAL(UTraversalActionComponent::GetMode, return EProjectCustomMovementMode::None;);
	virtual int32 GetPriority() const;
	virtual bool BuildCandidate(const FTraversalCandidate& )
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
