// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ControlShake.generated.h"

/**
 * 
 */

class UCurveVector;

USTRUCT(BlueprintType)
struct FControlShakeParams
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveVector> Curve =  nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator ShakeMagnitude = FRotator(1.f ,1.f, 1.f);
};

UCLASS()
class PROJECTFPS_API UControlShake : public UObject
{
	GENERATED_BODY()
	
public:
	void Activate(float InDuration, UCurveVector* InCurve, FRotator InShakeMagnitude);
	
	bool UpdateShake(float DeltaTime, FRotator& OutShake);
	
	void Clear();
	
	UPROPERTY()
	FControlShakeParams ControlShakeParams;
	
private:
	float TimeElapsed = 0.f;
	bool  bIsActive   = false;
};
