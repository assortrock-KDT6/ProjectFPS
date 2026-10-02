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

// 세션에서 선택하는 플레이 모드. 각 레벨의 게임 규칙과 별도로 관리한다.
UENUM(BlueprintType)
enum class EFPSMatchMode : uint8
{
	PVP = 0 UMETA(DisplayName = "PVP"),
	PVE UMETA(DisplayName = "PVE")
};

namespace FPSMatchModeUtils
{
	inline FString ToId(EFPSMatchMode Mode) { return Mode == EFPSMatchMode::PVE ? TEXT("PVE") : TEXT("PVP"); }

	inline bool TryParse(const FString& Id, EFPSMatchMode& Mode)
	{
		if (Id.Equals(TEXT("PVE"), ESearchCase::IgnoreCase))
		{
			Mode = EFPSMatchMode::PVE;
			return true;
		}
		// Existing quick-play Blueprint defaults used Deathmatch or an empty ID.
		if (Id.IsEmpty() || Id.Equals(TEXT("PVP"), ESearchCase::IgnoreCase)
			|| Id.Equals(TEXT("Deathmatch"), ESearchCase::IgnoreCase))
		{
			Mode = EFPSMatchMode::PVP;
			return true;
		}
		return false;
	}
}

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

UENUM(BlueprintType)
enum class  EProjectCustomMovementMode : uint8
{
	None = 0	UMETA(DisplayName = "NONE"),
	Vault		UMETA(DisplayName = "Vault"),
	Mantle		UMETA(DisplayName = "Mantle"),
	Hanging		UMETA(DisplayName = "Hanging")
};

UENUM(BlueprintType)
enum class ETraversalVariant : uint8
{
	Default		= 0	UMETA(DisplayName = "Default"),
	MantleLow	= 1	UMETA(DisplayName = "MantleLow"),
	MantleHigh	= 2	UMETA(DisplayName = "MantleHigh"),
	MantleInAir = 3 UMETA(DisplayName = "MantleInAir"),
};


// 아이템 종류
UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon	= 0,	// 무기 (가로형 슬롯)
	Ammo	= 1,	// 탄약 (정사각형 슬롯)
	Grenade = 2,	// 폭탄 (투척물)
	Healing = 3,	// 회복
	None	= 10,

};

// 장착 상태 on/off
// 장비창이 2개일 경우 ->x키로 파지 해제
// 장비칭이 1개일 경우 -> 비어있는 슬롯 으로 파지해제 
UENUM(BlueprintType)
enum class EItemState : uint8
{
	None
};

// OnlineSubsystemTypes.h 헤더 파일 참고해서 만듦.
namespace FCharacterStateUtils
{
	const TCHAR* ToString(EFPSOnlineOperationState Type);
	const TCHAR* ToString(EFPSOnlineConnectionState Type);
	const TCHAR* ToString(EFPSOnlineTravelState Type);
}
// Fill out your copyright notice in the Description page of Project Settings.
