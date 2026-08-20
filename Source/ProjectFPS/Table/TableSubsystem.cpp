// Fill out your copyright notice in the Description page of Project Settings.

#include "Table/TableSubsystem.h"	
#include "Table/TableDatas.h"		// FTablePathRow
#include "Kismet/GameplayStatics.h"	// UGameplayStatics::GetGameInstance


void UTableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);	// Super 부모 클래스의 Initialize 를 먼저 실행. (부모를 먼저 초기화함.)
	LoadTable();
	
}

void UTableSubsystem::Deinitialize()
{
	_Tables.Empty();		// 맵의 모든 항목을 비우는 것. 

	Super::Deinitialize();	// 마지막으로 부모를 정리함. 
}

UTableSubsystem* UTableSubsystem::Get(const UObject* WorldContext)
{
	if (nullptr == WorldContext)
		return nullptr;

	UGameInstance* Inst = UGameplayStatics::GetGameInstance(WorldContext); // 월드 게임 인스턴스 획득
	if (nullptr == Inst)
		return nullptr;

	return Inst->GetSubsystem<UTableSubsystem>();	// 그 인스턴스가 들고 있는 Subsystem을 반환.
}

bool UTableSubsystem::LoadTable()
{
	// 로더 테이블(TalbleLoader)를 먼저 로드
	_TablePath = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/Table/DT_TableLoader")));
	if (false == IsValid(_TablePath))
		return false;
	
	// 목록을 순회하며 IsUse의 테이블들을 로드해 등록.
	_TablePath->ForeachRow<FTablePathRow>(TEXT("TableLoader"),
		[this](const FName& Key, const FTablePathRow& Value)
		{
			if (Value._IsUse)
			{
				TObjectPtr<UDataTable> Loaded = Cast<UDataTable>
					(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Value._Path));
				if (Loaded)
					_Tables.Add(Key, Loaded);
			}
		});

	return true;
}
