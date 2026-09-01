// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Common/GameDefines.h"
#include "PacketDatas.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UPacketDatas : public UObject
{
	GENERATED_BODY()
	
};

/**
 * 서버가 확정하여 모든 클라이언트에 복제하는 최소 Traversal 상태.
 * UAnimMontage와 Warp Target Name은 Mode + Variant로 로컬 설정에서 조회한다.
 */

USTRUCT(BlueprintType)
struct FTraversalRepState
{
	GENERATED_BODY()

	UPROPERTY()
	EProjectCustomMovementMode _Mode = EProjectCustomMovementMode::None;

	UPROPERTY()
	uint8 _Variant = 0;

	/* 서버에서만 생성하고 증가시킨다. */
	UPROPERTY()
	uint16 _ActionID = 0;

	UPROPERTY()
	FVector_NetQuantize10 _TargetLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator _TargetRotation = FRotator::ZeroRotator;

	/**
	 * 각 머신에서 로컬 장애물 컴포넌트를 다시 찾기 위한 정보값.
	 */
	UPROPERTY()
	FVector_NetQuantize10 _ObstaclePoint = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal _ObstacleNormal = FVector::ZeroVector;

	UPROPERTY()
	float _ServerStartTimeSeconds = 0.f;

	UPROPERTY()
	float _Duration = 0.f;

	bool IsActive() const
	{
		return EProjectCustomMovementMode::None != _Mode && 0 != _ActionID && _Duration > 0.f;
	}

};