// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Table/TableDatas.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TableSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UTableSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	

protected:
	// TableLoader - uasset
	UPROPERTY()
	TObjectPtr<UDataTable> _TablePath;

	// 로드된 테이블 목록
	UPROPERTY()
	TMap<FName, TObjectPtr<UDataTable>> _Tables;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UTableSubsystem* Get(const UObject* WorldContext);

private:
	bool LoadTable();

public:
	TObjectPtr<UDataTable> FindTable(const FName& TableName) const
	{
		if (false == _Tables.Contains(TableName))	// TMap에 그 키가 있나? 
			return nullptr;							// 없으면 nullptr 반환.

		return *_Tables.Find(TableName);			// 값의 주소(포인터)를 반환 -> 값이 아닌 위치의 포인터로 실제 값을 꺼냄
	}

	template<typename T>
	const T* FindTableRow(const FName& TableName, const FName& RowName) const
	{
		TObjectPtr<UDataTable> table = FindTable(TableName);
		if (nullptr == table)
			return nullptr;
		return table->FindRow<T>(RowName, TEXT("not Found row"));
	}
};


