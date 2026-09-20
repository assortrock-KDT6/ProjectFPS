// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponRecoilPattern.generated.h"

/**
 * 
 */
class UCurveVector;

USTRUCT(BlueprintType)
struct FWeaponRecoilInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveVector> PatternSequence = nullptr;
	
	// 첫 패턴을 끝까지 사용한 뒤에 반복할 양끝을 포함한 구간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 LoopStartOffset = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 LoopEndOffset = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float RecoilOffsetResetTime = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveVector> SingleRecoilCurve = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.001"))
	float SingleRecoilDuration = 0.15f;
};

UCLASS()
class PROJECTFPS_API UWeaponRecoilPattern : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// Key에는 WeaponDataTable의 Row Name을 넣는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FWeaponRecoilInfo> Data;
	
	FVector GetRecoilPatternAt(FName WeaponID, int32 Offset) const;
};
