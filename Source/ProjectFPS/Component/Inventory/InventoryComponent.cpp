// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"



UInventoryComponent::UInventoryComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);		// 컴포넌트 복제 활성화.
}

// 아이템, 장비
bool UInventoryComponent::AddItem(FName TID, int32 Count)
{
	if (!GetOwner()->HasAuthority())	// 서버만 체크
		return false;
	if (TID.IsNone() || Count <= 0)
		return false;

	// 이미 있으면 개수 증가
	for (FInventorySlot& Slot : _Items)
	{
		if (Slot._TID == TID)
		{
			Slot._Count += Count;
			OnRep_Items();			// 서버는 OnRep 자동호출이 안되니 수동으로.
			return true;
		}
	}
	// 없으면 빈 칸에 채우기
	for (FInventorySlot& Slot : _Items)	// 아이템 10고정 슬롯횟수 반복 
	{
		if (Slot._TID.IsNone())			// 반복 중 비어있는 칸 발견
		{
			Slot._TID = TID;			// 빈 슬롯에 TID 정보를 넣음
			Slot._Count = Count;		// 수량 마찬가지
			OnRep_Items();				// 서버에 정보 전달.
			return true;
		}
	}
	// 빈칸 없음 -> 인벤토리가 가득참 (void -> bool) 
	return false;

}

bool UInventoryComponent::EquipItem(FName TID)
{
	if (!GetOwner()->HasAuthority())
		return false;
	if (TID.IsNone())
		return false;

	// 빈 무기 슬롯에 장착 (*교체 기능 추가해야함.)
	for (int32 i = 0; i < _Weapons.Num(); ++i)
	{ 

		if (_Weapons[i].IsNone())
		{
			_Weapons[i] = TID;
			OnRep_Weapons();
			return true;
		}
	}
	// 투척류

	// 방어구

	return false;

}


// 같은 역할 두개 묶기 -> enum -> 아이템 정보 가져오기.

FName UInventoryComponent::RemoveItem(int32 Index)
{
	return FName();
}

// 무기 슬롯 비우기
FName UInventoryComponent::RemoveWeapon(int32 Index)
{
	return FName();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, _Items);
	DOREPLIFETIME(UInventoryComponent, _Weapons);
}


void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 무기 슬롯 초기화 
	if (GetOwner()->HasAuthority())
	{
		_Weapons.SetNum(2);	// 장비 1,2 슬롯
		_Items.SetNum(10);	// 아이템 창 10 슬롯
	}
}

void UInventoryComponent::OnRep_Items()
{
}

void UInventoryComponent::OnRep_Weapons()
{
}