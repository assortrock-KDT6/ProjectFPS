// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Common/GameDatas.h"
#include "InventoryComponent.generated.h"


// 소지품 데이터 서버만 수정 및 복제로 클라에 전파함.
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

	// 손에 장착 중인 무기의 슬롯 번호(없으면 INDEX_NONE). 버릴 슬롯과는 무관.
	UPROPERTY(Replicated)
	int32 _EquippedWeaponIndex = INDEX_NONE; // 기본 장착무기 없음 -> INEXT_NONE

	// 장비교체
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanger _OnInventoryChanged;

public:
	bool AddItem(FName TID, int32 Count = 1);
	bool EquipItem(FName TID);
	bool TryAcquire(FName TID, int32 Count = 1);
	bool FindEquipSlot(FName TID, int32& OutSlotIndex, FName& OutReplacedITD) const;

	// FindEquipSlot 결과를 반영 (서버). 슬롯을 채우고 장착 슬롯으로 지정함.
	bool CommitEquip(FName TID, int32 SlotIndex, bool bNotify);
	void NotifyWeaponsChanged() { OnRep_Weapons(); }
	// 슬롯을 비운다 - 장비 서버전용
	bool RemoveWeapon(int32 Index);

	// 슬롯에서 Count만큼 뺀다. 소모품 서버전용
	bool RemoveItem(int32 Index, int32 Count);

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
	
	// 현재 장착한 무기의 슬롯 번호를 반환함.
	int32 GetEquippedWeaponIndex() const { return _EquippedWeaponIndex; }

	// 지정한 무기 슬롯의 TID 반환. 범위 밖이면 NAME_None 반환.
	FName GetWeaponTID(int32 Index) const 
	{
		// 슬롯 번호가 배열 범위 박이면 반환
		if (false == _Weapons.IsValidIndex(Index))
			return NAME_None;

		// 해당 슬롯의 무기 TID 반환
		return _Weapons[Index];
	}
	
	bool IsWeaponSlotFull() const;

	// 장착 슬롯을지정 (서버) 범위 밖이면 INDEX_NONE 처리.
	void SetEquippedWEaponIndex(int32 Index);

	// 비어 있지 않은 첫 무기 슬롯 
	int32 FindFirstWeaponSlot() const;

	

};
