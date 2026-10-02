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

	UPROPERTY()
	FName _TID;

	// 보유 개수
	UPROPERTY()
	int32 _Count = 1;

	// 배열 인덱스(슬롯 위젯이 자기 번호를 알기 위함.)
	UPROPERTY()
	int32 _Index = INDEX_NONE;

};
