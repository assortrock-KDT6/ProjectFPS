// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/GrenadeStatusWidget.h"
#include "Component/Inventory/InventoryComponent.h"
#include "GameMode/PlayerStateBase.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h" 
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"


void UGrenadeStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 인벤토리를 찾기 전까지는 숨김.
	SetVisibility(ESlateVisibility::Collapsed);
	TryBindInventory();

}

void UGrenadeStatusWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(_BindRetryHandle);
	}

	UnBindInventory();

	Super::NativeDestruct();

}

void UGrenadeStatusWidget::TryBindInventory()
{
	UInventoryComponent* Inventory = nullptr;

	// pc -> ps -> 인벤
	APlayerController* Pc = GetOwningPlayer();
	if (nullptr != Pc)
	{
		APlayerStateBase* Ps = Pc->GetPlayerState<APlayerStateBase>();
		if (nullptr != Ps)
		{
			Inventory = Ps->GetInventory();
		}
	}

	// 클라는 PlayerState가 위젯보다 늦게 복제될 수 있어서 잠시 후 다시 시도.
	if (nullptr == Inventory)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(_BindRetryHandle, this, &UGrenadeStatusWidget::TryBindInventory, _BindRetryInterval, false);
		}
		return;
	}
	
	UnBindInventory();

	_BoundInventory = Inventory;
	Inventory->_OnInventoryChanged.AddUniqueDynamic(this, &UGrenadeStatusWidget::UpdateGrenadeStatus);

	// 구독 전에 이미 가진 수류탄도 첫 화면에 표시
	UpdateGrenadeStatus();
}

void UGrenadeStatusWidget::UnBindInventory()
{
	if (true == _BoundInventory.IsValid())
	{
		_BoundInventory->_OnInventoryChanged.RemoveDynamic(this, &UGrenadeStatusWidget::UpdateGrenadeStatus);
	}

	_BoundInventory.Reset();
}

void UGrenadeStatusWidget::UpdateGrenadeStatus()
{
	UInventoryComponent* Inventory = _BoundInventory.Get();
	UTableSubsystem* Table = UTableSubsystem::Get(this);
	if (nullptr == Inventory || nullptr == Table)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 소모품 중 ItemTable 타입이 Grenade인 칸의 수량을 모두 합산
	int32 Count = 0;
	UTexture2D* Icon = nullptr;

	for (const FInventorySlot& Item : Inventory->GetItems())
	{
		if (Item._TID.IsNone() || Item._Count <= 0)
		{
			continue;
		}

		const FItemData* Row = Table->FindTableRow<FItemData>(TEXT("ItemTable"), Item._TID);
		if (nullptr == Row || EItemType::Grenade != Row->_ItemType)
		{
			continue;
		}

		Count += Item._Count;

		// 아이콘은 처음 찾은 수류탄 기준
		if (nullptr == Icon)
		{
			Icon = Row->_Icon;
		}
	}

	// 수류탄이 없으면 숨긴다.
	if (Count <= 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (nullptr != _IconImage && nullptr != Icon)
	{
		_IconImage->SetBrushFromTexture(Icon, true);
	}

	if (nullptr != _CountText)
	{
		_CountText->SetText(FText::AsNumber(Count));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}
