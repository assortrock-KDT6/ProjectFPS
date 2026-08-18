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
};

#pragma endregion

