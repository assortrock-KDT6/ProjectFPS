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

UENUM(BlueprintType)
enum class EKillLogRelation : uint8
{
	Unknown,
	Self,
	Ally,
	Enemy
};

USTRUCT(BlueprintType)
struct PROJECTFPS_API FPlayerKillLogResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	int32 _KillerPlayerId = INDEX_NONE;

	// 킬 당시의 팀/경기 규칙. 이후 팀 변경이나 퇴장에도 기록은 바뀌지 않는다.
	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	int32 _KillerTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	bool _bTeamMatch = false;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	FString _KillerPlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	FString _KilledPlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	UTexture2D* _WeaponIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "KillLog")
	FLinearColor _WeaponIconTint = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "KillIcon")
	EWeaponType _KillerWeaponType = EWeaponType::None;

	bool IsValid() const
	{
		return false == _KillerPlayerName.IsEmpty() && false == _KilledPlayerName.IsEmpty();
	}

	EKillLogRelation ResolveRelation(int32 LocalPlayerId, int32 LocalTeamId) const
	{
		if (_KillerPlayerId < 0 || LocalPlayerId < 0)
		{
			return EKillLogRelation::Unknown;
		}
		if (_KillerPlayerId == LocalPlayerId)
		{
			return EKillLogRelation::Self;
		}
		if (!_bTeamMatch)
		{
			return EKillLogRelation::Enemy;
		}
		if (_KillerTeamId < 0 || LocalTeamId < 0)
		{
			return EKillLogRelation::Unknown;
		}
		return _KillerTeamId == LocalTeamId ? EKillLogRelation::Ally : EKillLogRelation::Enemy;
	}
};
