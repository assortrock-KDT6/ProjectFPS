// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataTable.h"
#include "Common/GameDefines.h"
#include "GameFramework/Actor.h"
#include "TableDatas.generated.h"


/**
 * 
 */
UCLASS()
class PROJECTFPS_API UTableDatas : public UObject
{
	GENERATED_BODY()
	
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

// 아이템 테이블 행
USTRUCT(BlueprintType)
struct FItemData  : public  FTableRowBase
{
	GENERATED_BODY()

	// 아이템의 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText _DisplayName;

	// 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText _Description;
	
	// 아이템 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> _Icon = nullptr;
	
	// 바닥에 놓인 아이템의 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> _WorldMesh = nullptr;

	// 아이템 타입.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType _ItemType = EItemType::None;

	// 아이템 최대 수량.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 _MaxStackcount = 1;

	// 손에 장착할 Actor 클래스

	
};