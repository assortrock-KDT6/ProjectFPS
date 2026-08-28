// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataTable.h"
#include "TableDatas.generated.h"


/**
 * 
 */
UCLASS()
class PROJECTFPS_API UTableDatas : public UObject
{
	GENERATED_BODY()
	
};

// 아이템 테이블 행
// 언리얼 Struct 
USTRUCT(BlueprintType)
struct FItemData  : public  FTableRowBase
{
	GENERATED_BODY()

	// 아이템 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName _ItemId;

	// 아이템 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> _Icon = nullptr;			

	//  *나중에 지울예정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 _Count = 1;
};

// 테이블 목록 행
USTRUCT(BlueprintType)
struct FTablePathRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString _Path;		// 그 테이블 에셋 경로

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool _IsUse = true; // 로드 여부
	
};
