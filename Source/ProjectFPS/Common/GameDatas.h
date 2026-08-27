// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

