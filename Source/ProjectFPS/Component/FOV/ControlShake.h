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
	
	// 참이면 카메라 반동, 거짓이면 총과 손의 시각적 반동으로 계산
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAffectCamera = true;
};

UCLASS()
class PROJECTFPS_API UControlShake : public UObject
{
	GENERATED_BODY()
	
public:
	void Activate(const FControlShakeParams& InParams);
	
	bool UpdateShake(float DeltaTime, FRotator& OutShake);
	
	void Clear();
	
	UPROPERTY()
	FControlShakeParams ControlShakeParams;
	
private:
	float TimeElapsed = 0.f;
	bool  bIsActive   = false;
};
