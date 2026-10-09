// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/InventoryWidget.h"
#include "UI/GamePlay/ItemObject.h"
#include "UI/GamePlay/ItemSlotWidget.h"
#include "UI/GamePlay/ItemInfoWidget.h"
#include "UI/GamePlay/DropCountWidget.h"
#include "Character/CharacterPlayer.h"
#include "GameMode/PlayerStateBase.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Components/TileView.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// WBP에 고정 배치된 슬롯은 지금바로 
	BindSlot(_MainWeaponSlot);
	BindSlot(_SubWeaponSlot);

	// 고정 슬롯에 타입/번호 부여.
	if (_MainWeaponSlot)
	{
		_MainWeaponSlot->SetSlotId(EItemType::Weapon, 0);
	}
	
	if (_SubWeaponSlot)
	{
		_SubWeaponSlot->SetSlotId(EItemType::Weapon, 1);
	}

	// TileView 슬롯은 런타임 생성 
	if (nullptr != _ItemTileView)
		_ItemTileView->OnEntryWidgetGenerated().AddUObject(this, &UInventoryWidget::HandleEntryGenerated);

	if (_DropCountPanel)
		_DropCountPanel->_OnConfirmed.AddUniqueDynamic(this, &UInventoryWidget::HandleDropCountConfirmed);

	// 인벤토리 변경 신호 -> 창 열려 있는 동아 줍기 버리기 결과가 반영됨.
	if (UInventoryComponent* Inv = FindInventory())
		Inv->_OnInventoryChanged.AddUniqueDynamic(this, &UInventoryWidget::HandleInventoryChanged);

	Refresh();
}

void UInventoryWidget::NativeDestruct()
{
	// 창 닫힐 때 구독 해재(안하면 지워진 위젯으로 방송이 감)
	if (UInventoryComponent* Inv = FindInventory())
		Inv->_OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);

	Super::NativeDestruct();
}

void UInventoryWidget::BindSlot(UItemSlotWidget*  InSlot)
{
	if (nullptr == InSlot)
		return;

	InSlot->SetInfoPanel(_ItemInfoPanel);
	InSlot->_OnDropRequested.AddUniqueDynamic(this, &UInventoryWidget::HandleSlotDropRequested); 
}

void UInventoryWidget::HandleEntryGenerated(UUserWidget& EntryWidget)
{
	BindSlot(Cast<UItemSlotWidget>(&EntryWidget));
}

void UInventoryWidget::HandleInventoryChanged()
{
	Refresh();
}

UInventoryComponent* UInventoryWidget::FindInventory() const
{
	APlayerController* Pc = GetOwningPlayer();
	if (nullptr == Pc)
		return nullptr;

	APlayerStateBase* Ps = Pc->GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return nullptr;

	return Ps->GetInventory();

}

void UInventoryWidget::Refresh()
{
	if (nullptr == _ItemTileView)
		return;
	_ItemTileView->ClearListItems();

	UInventoryComponent* Inv = FindInventory();
	if (nullptr == Inv)
		return;

	// 소모품 슬롯으로
	const TArray<FInventorySlot>& Items = Inv->GetItems();
	for (int32 i = 0; i< Items.Num(); ++i)
	{
		UItemObject* Obj = NewObject<UItemObject>(this);
		Obj->_TID = Items[i]._TID;
		Obj->_Count = Items[i]._Count;
		Obj->_Index = i;
		_ItemTileView->AddItem(Obj);
	}
	
	// 무기 슬롯으로
	const TArray<FWeaponSlotData>& Weapons = Inv->GetWeapons();
	if (_MainWeaponSlot)
		_MainWeaponSlot->SetSlot(Weapons.IsValidIndex(0) ? Weapons[0]._WeaponId : NAME_None);

	if (_SubWeaponSlot)
		_SubWeaponSlot->SetSlot(Weapons.IsValidIndex(1) ? Weapons[1]._WeaponId : NAME_None);

}

void UInventoryWidget::HandleSlotDropRequested(EItemType Type, int32 Index, int32 Count)
{
	// 무기 1개 
	if (EItemType::Weapon == Type || Count <= 1)
	{
		RequestDrop(Type, Index, 1);
		return;
	}

	// 소모품 여러 개 : 수량 패널, 패널이 없을 경우 전부 버림.
	if (_DropCountPanel)
		_DropCountPanel->Open(Index, Count);
	else
		RequestDrop(Type, Index, Count);
}

void UInventoryWidget::HandleDropCountConfirmed(int32 Index, int32 Count)
{
	RequestDrop(EItemType::None,Index, Count); // 소모품.
}

void UInventoryWidget::RequestDrop(EItemType Type, int32 Index, int32 Count)
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetOwningPlayerPawn());
	if (nullptr == Character)
		return;

	if (EItemType::Weapon == Type)
		Character->ServerDropWeaponAt(Index);
	else
		Character->ServerDropItemAt(Index, Count);

}