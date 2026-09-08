// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/InventoryWidget.h"
#include "UI/GamePlay/ItemObject.h"
#include "UI/GamePlay/ItemSlotWidget.h"
#include "GameMode/PlayerStateBase.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Components/TileView.h"


void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Refresh();
}

void UInventoryWidget::Refresh()
{
	if (nullptr == _ItemTileView)
		return;
	_ItemTileView->ClearListItems();

	APlayerController* PC = GetOwningPlayer();
	if (nullptr == PC)
		return;

	APlayerStateBase* PS = PC->GetPlayerState<APlayerStateBase>();
	if (nullptr == PS)
		return;

	UInventoryComponent* Inv = PS->GetInventory();
	if (nullptr == Inv)
		return;

	// 소모품 슬롯으로
	for (const FInventorySlot& InvSlot : Inv->GetItems()) 
	{
		UItemObject* Obj = NewObject<UItemObject>(this);
		Obj->_TID = InvSlot._TID;
		Obj->_Count = InvSlot._Count;
		_ItemTileView->AddItem(Obj);
	}
	
	// 무기 슬롯으로
	const TArray<FName>& Weapons = Inv->GetWeapons();
	if (_WeaponSlot1)
		_WeaponSlot1->SetSlot(Weapons.IsValidIndex(0) ? Weapons[0]: NAME_None);

	if (_WeaponSlot2)
		_WeaponSlot2->SetSlot(Weapons.IsValidIndex(1) ? Weapons[1] : NAME_None);

}
