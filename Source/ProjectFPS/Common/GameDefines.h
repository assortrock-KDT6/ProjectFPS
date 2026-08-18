// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameDefines.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UGameDefines : public UObject
{
	GENERATED_BODY()

};

// 언리얼 enun

// 인게임 화면
UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	Waiting,	// 대기화면
	GamePlay	// 게임 플레이

};


// 

UENUM(BlueprintType)
enum class EFPSOnlineSessionState : uint8
{
	Idle,
	Destroying,
	Creating,
	Finding,
	Joining,
	Traveling
};


