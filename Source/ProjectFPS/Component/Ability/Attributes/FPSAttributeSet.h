// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "FPSAttributeSet.generated.h"

/**
 * 
 */
class UFPSAbilitySystemComponent;

UCLASS()
class PROJECTFPS_API UFPSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UFPSAttributeSet();
public:
	UWorld*						GetWorld() const override;
	UFPSAbilitySystemComponent* GetFPSAttributeComponent() const;
};
