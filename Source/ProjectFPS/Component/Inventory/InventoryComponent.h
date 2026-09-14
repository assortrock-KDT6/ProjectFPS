// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Common/GameDatas.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanger);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

private:
	// 소모품 슬롯
	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FInventorySlot> _Items;

	// 장비 슬롯
	UPROPERTY(ReplicatedUsing = OnRep_Weapons)
	TArray<FName> _Weapons;

	// 장비교체
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanger _OnInventoryChanged;

public:
	// Add -> 
	bool AddItem(FName TID, int32 Count = 1);
	bool EquipItem(FName TID);
	bool TryAquire(FName TID, int32 Count = 1);

	FName RemoveWeapon(int32 Index);
	FName RemoveItem(int32 Index);

	// 복제할 변수 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 복제되면 클라에서 호출 UI 갱신
	UFUNCTION()
	void OnRep_Items();

	UFUNCTION()
	void OnRep_Weapons();

public:
	const TArray<FInventorySlot>& GetItems() const { return _Items; }
	const TArray<FName>& GetWeapons() const { return _Weapons; }

	

};
