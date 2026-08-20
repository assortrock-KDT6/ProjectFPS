// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/InventoryWidget.h"
#include "UI/GamePlay/ItemObject.h"
#include "Gamemode/PlayerStateBase.h"
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

	// 보유 아이템 순회확인 -> UItemObject -> 타일에 추가
	for(auto&  Pair : PS->GetItems())
	{
		UItemObject* Obj = NewObject<UItemObject>(this);
		Obj->_TID = Pair.Key;
		Obj->_Count = Pair.Value;
		_ItemTileView->AddItem(Obj);
	}
		
}
