// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameDefines.h"
#include "UObject/Object.h"
#include "GameDatas.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UGameDatas : public UObject
{
	GENERATED_BODY()
	
};

#pragma region SessionData

USTRUCT(BlueprintType)
struct FFPSOnlineSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	int32 _ResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	FString _SessionOwnerName;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	FString _MapName;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	int32 _PingInMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	int32 _CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	int32 _MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	bool _IsLan = false;

	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	FString	_DisplayName;
	
	UPROPERTY(BlueprintReadOnly, Category = "OnlineSession")
	FString _GameModeId;
};

/* 방 만들 때 쓰이는 옵션 구조체. */
USTRUCT(BlueprintType)
struct FFPSSessionCreateOptions
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnlineSession", meta = (ClampMin = "1"))
	int32 _MaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnlineSession")
	bool _AllowJoinProgress = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnlineSession")
	FString _MapId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnlineSession")
	FString _DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnlineSession")
	FString _GameModeId;
};

// 아이템 슬롯.
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName _TID= NAME_None;
	UPROPERTY(BlueprintReadOnly) int32 _Count = 0;

};


#pragma endregion


/**
 * Front / Top처럼 모든 Traversal 액션이 공유하는 서버 로컬 Trace 결과다.
 * 포인터와 FHitResult를 포험하므로 네트워크로 복제하지 않는다.
 */
struct FTraversalBaseQuery
{
	FHitResult	_FrontHit;

	FHitResult	_TopHit;

	FVector		_Direction = FVector::ZeroVector;
	
	float		_ObstacleHeight = 0.f;

	bool IsValid() const
	{
		return	_FrontHit.IsValidBlockingHit() && _TopHit.IsValidBlockingHit() && !_Direction.IsNearlyZero();
	}
};

/**
 * 서버의 액션 컴포넌트가 생성하고 MovementComponent에 전달하는 로컬 후보 결과.
 */
struct FTraversalCandidate
{
	EProjectCustomMovementMode	_Mode = EProjectCustomMovementMode::None;

	uint8						_Variant = 0;

	FVector						_TargetLocation = FVector::ZeroVector;

	FRotator					_TargetRotation = FRotator::ZeroRotator;

	FVector						_ObstaclePoint = FVector::ZeroVector;

	FVector						_ObstacleNormal = FVector::ZeroVector;;

	TWeakObjectPtr<UPrimitiveComponent> _ObstacleComponent = nullptr;

	float	_Duration = 0.f;

	bool IsValid() const
	{
		return  EProjectCustomMovementMode::None != _Mode && _Duration > 0.f;
	}
};

/**
 * 로컬 액션 설정. 런타임에 Montage 자체를 복제하지 않고 Mode + Variant로 조회한다.
 */

USTRUCT(BlueprintType)
struct FTraversalActionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EProjectCustomMovementMode _Mode = EProjectCustomMovementMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 _Variant = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> _Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float _PlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName _WarapTargetName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float _MinHeight = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float _MaxHeight = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float _MaxDepth = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float _BlendOutTime = 0.1f;
};

USTRUCT(BlueprintType)
struct FVaultTraceSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float _BackCheckDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float _BackCheckHeight = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float _LandingForwardOffset = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float _LandingTraceUp = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float _LandingTraceDown = 300.f;
};

USTRUCT(BlueprintType)
struct FMantleTraceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float _TopFloorTraceHalfDistance = 50.f;
};
