// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemObject.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UItemObject : public UObject
{
	GENERATED_BODY()
	
public:

	// 테이블 조회 키s
	UPROPERTY()
	FName _TID;

	// 보유 개수
	UPROPERTY()
	int32 _Count = 1;

};
