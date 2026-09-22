// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Inventory/InventoryComponent.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"
#include "Common/GameDefines.h"
#include "Net/UnrealNetwork.h"



UInventoryComponent::UInventoryComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);		// 컴포넌트 복제 활성화.
}

bool UInventoryComponent::EquipItem(FName TID)
{
	// 인벤토리 컴포넌트를 소유한 액터가 서버 권한을 가지고 있는지 확인.
	if (!GetOwner()->HasAuthority())
		return false;

	if (TID.IsNone())
		return false;

	// 빈 무기 슬롯에 장착하고, 그 칸을 장착 슬롯으로
	for (int32 i = 0; i < _Weapons.Num(); ++i)	// 무기 슬롯을 확인 main,sub 장비칸이 2개니까 
	{
		// 장비칸 2개중 빈슬롯이 있는지 확인 
		if (_Weapons[i].IsNone())
		{
			// 비어있는 슬롯을 찾아서 TID(무가) 넣어줌.
			_Weapons[i] = TID;
			_EquippedWeaponIndex = i;
			OnRep_Weapons();	// 서버에 전달.
			return true;		
		}
	}

	return false;	// 장비창에 빈자리가 없음	-> 교체로 호출해야함.
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



bool UInventoryComponent::TryAcquire(FName TID, int32 Count)
{
	// 서버가 아니면 거부 아이템 획득은 서버만 결정하고, 결과를 복제로 내보냄 -> 클라를 막음.
	if (false == GetOwner()->HasAuthority())
		return false;

	// TID가 없거나 0보다 수량이 적을 경우 실패처리
	if (TID.IsNone() || Count <= 0)
		return false;

	// 테이블 조회를 위함. 
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return false;

	// 테이블에 TID로 행 찾기 없거나 오타 -> 실패처리
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);
	if (nullptr == Row)
		return false;
	
	if (EItemType::Weapon == Row->_ItemType)
		return EquipItem(TID);

	return AddItem(TID, Count);

}

bool UInventoryComponent::FindEquipSlot(FName TID, int32& OutSlotIndex, FName& OutReplacedITD) const
{
	OutSlotIndex = INDEX_NONE;	// 선택한 슬롯 없음
	OutReplacedITD = NAME_None;	// 밀려날 아이템 없음

	if (TID.IsNone() || _Weapons.Num() <= 0)
		return false;
	
	// 빈 슬롯이 있으면 진입.
	for (int32 i = 0; i < _Weapons.Num(); ++i)
	{
		if (_Weapons[i].IsNone())
		{
			OutSlotIndex = i;
			return true;
		}
	}

	// 꽉 참 -> 장착 슬롯을 교체 (없을 경우 0번으로)
	if (_Weapons.IsValidIndex(_EquippedWeaponIndex))
	{
		OutSlotIndex = _EquippedWeaponIndex;
	}
	else
	{
		// 유효한 장착 슬롯이 없으면 0번 선택
		OutSlotIndex = 0;
	}
	// 선택한 슬롯에서 밀려날 아이템TID를 가져옴.
	OutReplacedITD = _Weapons[OutSlotIndex];
	return true;
}

// 서버전용 슬롯을 비운다 장비템
bool UInventoryComponent::RemoveWeapon(int32 Index)
{
	// 서버 권한체크 
	if (!GetOwner()->HasAuthority())
		return false;

	// 슬롯 번호가 범위 밖이거나(슬롯범위는 Main,Sub 장비창 2개를 뜻함.), 해당 슬롯이 이미 비어 있으면 실패.
	if (false == _Weapons.IsValidIndex(Index) || _Weapons[Index].IsNone())
		return false;

	_Weapons[Index] = NAME_None;

	//손에 들고 있던 걸 빼면 장착 슬롯 해제
	if (Index == _EquippedWeaponIndex)
		_EquippedWeaponIndex = INDEX_NONE;

	OnRep_Weapons();
	return true;


}

// 서버전용 슬롯을 비운다 소모품
bool UInventoryComponent::RemoveItem(int32 Index, int32 Count)
{
	// 서버 권한체크 
	if (!GetOwner()->HasAuthority())
		return false;

	// 슬롯 번호와 제거할 수량 확인
	if (false == _Items.IsValidIndex(Index) || Count <= 0)
		return false;

	// 해당 슬롯 참조.
	FInventorySlot& Slot = _Items[Index];
	// 아이템과 보유 수량확인.
	if (Slot._TID.IsNone() || Slot._Count < Count)
		return false;
	// 수량 차감
	Slot._Count -= Count;
	// 차감이 불가능 할 경우 초기화(빈슬롯)
	if (Slot._Count <= 0)
		Slot = FInventorySlot();

	OnRep_Items();
	return true;
	
}

bool UInventoryComponent::CommitEquip(FName TID, int32 SlotIndex, bool bNotify)
{
	if (false == GetOwner()->HasAuthority())
		return false;

	if (TID.IsNone() || false == _Weapons.IsValidIndex(SlotIndex))
		return false;

	_Weapons[SlotIndex] = TID;
	_EquippedWeaponIndex = SlotIndex;
	if (bNotify)
	{
		OnRep_Weapons();
	}
	return true;
}

bool UInventoryComponent::IsWeaponSlotFull() const
{
	for(const FName& Slot : _Weapons)
	{
		if (Slot.IsNone())
			return false;
	}
	return _Weapons.Num() > 0;
}

void UInventoryComponent::SetEquippedWEaponIndex(int32 Index)
{
	// 장착 슬롯 번호 기록
	
	// 서버 권한 확인.
	if (!GetOwner()->HasAuthority())
		return;

	// 현재 장착중인 슬롯 번호
	if (_Weapons.IsValidIndex(Index))	// 비어있는지 확인 후 비어있으면 채워줌
	{
		_EquippedWeaponIndex = Index;
	}
	else
	{
		_EquippedWeaponIndex = INDEX_NONE;
	}

	OnRep_Weapons();
}

int32 UInventoryComponent::FindFirstWeaponSlot() const
{
	// 무기가 있는 첫 슬롯 찾기

	for (int32 i = 0; i < _Weapons.Num(); ++i)
	{
		if (false == _Weapons[i].IsNone())
			return i;
	}
	return INDEX_NONE;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// 인벤토리의 어떤 변수를 서버에서 클라이언트로 복제할지 등록하는 함수입니다
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, _Items);
	DOREPLIFETIME(UInventoryComponent, _Weapons);
	DOREPLIFETIME(UInventoryComponent, _EquippedWeaponIndex);

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

