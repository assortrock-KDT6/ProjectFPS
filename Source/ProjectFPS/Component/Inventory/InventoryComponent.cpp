// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"



UInventoryComponent::UInventoryComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);		// 컴포넌트 복제 활성화.
}


void UInventoryComponent::AddItem(FName TID, int32 Count)
{
	if (!GetOwner()->HasAuthority())	// 서버만 체크
		return;
	if (TID.IsNone() || Count <= 0)
		return;

	// 이미 있으면 개수 증가
	for (FInventorySlot& Slot : _Items)
	{
		if (Slot._TID == TID)
		{
			Slot._Count += Count;
			OnRep_Items();			// 서버는 OnRep 자동호출이 안되니 수동으로.
			return;
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
			return;
		}
	}
	// 빈칸 없음 -> 인벤토리가 가득참 (예정) 

}

void UInventoryComponent::EquipWeapon(FName TID)
{
	if (!GetOwner()->HasAuthority())
		return;
	if (TID.IsNone())
		return;

	// 빈 무기 슬롯에 장착 (*교체 기능 추가해야함.)
	for (int32 i = 0; i < _Weapons.Num(); ++i)
	{

		if (_Weapons[i].IsNone())
		{
			_Weapons[i] = TID;
			OnRep_Weapons();
			return;
		}
	}

	// 교체기능 (*예정)
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