#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "PlayerMatchStats.generated.h"

/** 리스폰과 무관하게 경기 동안 누적되는 개인 기록. */
USTRUCT(BlueprintType)
struct PROJECTFPS_API FPlayerMatchStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 _KillScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 _DeathScore = 0;
};

/** 경기 종료 시점의 복사본. 이후 참가자가 퇴장해도 결과가 유지된다. */
USTRUCT(BlueprintType)
struct PROJECTFPS_API FPlayerMatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 _PlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	FString _PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	FPlayerMatchStats _Stats;

	// Kill이 같으면 공동 순위다. Death는 순위에 영향을 주지 않는다.
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 _Rank = 0;
};

USTRUCT(BlueprintType)
struct PROJECTFPS_API FPlayerKillLogResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	FString _KillerPlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	FString _KilledPlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	UTexture2D* _WeaponIcon = nullptr;



	bool IsValid() const
	{
		return false == _KillerPlayerName.IsEmpty() && false == _KilledPlayerName.IsEmpty();
	}
};
