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

// 현재 실행 중인 비동기 작업 상태.
UENUM(BlueprintType)
enum class EFPSOnlineOperationState : uint8
{
	Idle = 0	UMETA(DisplayName = "IDLE"),
	Destroying	UMETA(DisplayName = "DESTROYING"),
	Creating	UMETA(DisplayName = "CREATING"),
	Finding		UMETA(DisplayName = "FINDING"),
	Joining		UMETA(DisplayName = "JOINING"),
	Starting	UMETA(DisplayName = "STARTING"),
	Ending		UMETA(DisplayName = "ENDING")
};

// 현재 세션 접속 상태.
UENUM(BlueprintType)
enum class EFPSOnlineConnectionState : uint8
{
	None = 0		UMETA(DisplayName = "NONE"),
	Hosting			UMETA(DisplayName = "HOSTING"),
	Joined			UMETA(DisplayName = "JOINED"),
	CleanupFailed	UMETA(DisplayName = "CLEANUP_FAILED")
};

// World 이동 상태.
UENUM(BlueprintType)
enum class EFPSOnlineTravelState : uint8
{
	None = 0	UMETA(DisplayName = "NONE"),
	Traveling	UMETA(DisplayName = "TRAVELING")
};

// 아이템 종류 --> 
UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon,		// 무기 (가로형 슬롯)
	Consumable,	// 소모품 (정사각형 슬롯)
	Bullet,		// 탄약 (정사각형 슬롯)
};



// OnlineSubsystemTypes.h 헤더 파일 참고해서 만듦.
namespace FCharacterStateUtils
{
	const TCHAR* ToString(EFPSOnlineOperationState Type);
	const TCHAR* ToString(EFPSOnlineConnectionState Type);
	const TCHAR* ToString(EFPSOnlineTravelState Type);
}