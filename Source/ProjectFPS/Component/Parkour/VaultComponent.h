// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Parkour/TraversalActionComponent.h"
#include "VaultComponent.generated.h"

struct FTraversalQuery;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UVaultComponent : public UTraversalActionComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVaultComponent();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Vault|Trace")
	FVaultTraceSettings _TraceSettings;

public:
	virtual EProjectCustomMovementMode GetMode() const override;
	virtual int32	GetPriority() const override;
	virtual bool	BuildCandidate(const FTraversalBaseQuery& BaseQuery, FTraversalCandidate& OutCandidate) const;

};
