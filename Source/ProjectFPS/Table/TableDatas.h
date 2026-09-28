// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataTable.h"
#include "Common/GameDefines.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> _Icon = nullptr;			 

	// 메시 정보 
	UPROPERTY(EditAnyWhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> _WorldMesh = nullptr;

	// 아이템 종류
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemType _ItemType = EItemType::None;

	//  아이템 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 _Count = 1;
	
	// 아이템 테이블과 무기를 연결하는 외래키
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName _WeaponId = NAME_None;

	// 버릴 떄 생성할 픽업 클래스
	// 지정한 BP의 기본 설정과 기능을 적용하여 월드에 생성함.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class AItemPickUp> _PickUpClass;	// 부모클래스 일 경우 자식클래스도 포함됨.
};



