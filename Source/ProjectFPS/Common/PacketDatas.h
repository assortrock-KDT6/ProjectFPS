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
 * UFPSCharacterMovementComponent::_TraversalData 하나로 복제되므로 각 구조체 멤버를
 * DOREPLIFETIME에 개별 등록할 필요는 없다.
 */
USTRUCT(BlueprintType)
struct FC_TraversalData
{
	GENERATED_BODY()

	UPROPERTY()
	EProjectCustomMovementMode _Mode = EProjectCustomMovementMode::None;

	/** 
	 * 0: 기본, 1 : 높은 Mantle 등, 모드별 해석은 서버와 클라이언트가 공유한다.
	 */
	UPROPERTY()
	uint8 _Variant = 0;

	/**
	 * 이전 Traversal의 늦은 OnRep와 현재 액션을 구분한다. 
	 */
	UPROPERTY()
	uint16 _ActionId = 0;

	UPROPERTY()
	FVector_NetQuantize10 _StartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 _TargetLocation = FVector::ZeroVector;

	// 서버 FrontHit의 충돌 위치, 클라이언트 호컬 재탐색의 기준점.
	UPROPERTY()
	FVector_NetQuantize10 _ObstaclePoint = FVector::ZeroVector;

	// 방향 벡터 전용 양자화 타입을 사용한다.
	UPROPERTY()
	FVector_NetQuantizeNormal _ObstacleNormal = FVector::ZeroVector;

	UPROPERTY()
	FRotator _TargetRotation = FRotator::ZeroRotator;

	UPROPERTY()
	FName _WarpTargetName = NAME_None;

	UPROPERTY()
	float _ExpectedDuration = 0.f;

	// AGameStateBase::GetServerWorldTimeSeconds() 기준이다.
	UPROPERTY()
	float _ServerStartTimeSeconds = 0.f;

	bool IsValid() const
	{
		return	EProjectCustomMovementMode::None != _Mode
				&& 0 != _ActionId
				&& NAME_None != _WarpTargetName
				&& _ExpectedDuration > 0.f;
	}
};