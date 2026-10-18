// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/GrenadeStatusWidget.h"
#include "Character/CharacterPlayer.h"
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

	// 고른 투척물 종류는 캐릭터가 갖고 있어서, 폰이 바뀔 때마다 다시 연결한다.
	_BoundPlayerController = GetOwningPlayer();
	if (true == _BoundPlayerController.IsValid())
	{
		_BoundPlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UGrenadeStatusWidget::HandlePawnChanged);
	}
	BindCharacter(GetOwningPlayerPawn());

	TryBindInventory();

}

void UGrenadeStatusWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(_BindRetryHandle);
	}

	if (true == _BoundPlayerController.IsValid())
	{
		_BoundPlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UGrenadeStatusWidget::HandlePawnChanged);
	}
	_BoundPlayerController.Reset();

	UnBindCharacter();
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

void UGrenadeStatusWidget::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindCharacter(NewPawn);
	UpdateGrenadeStatus();
}

void UGrenadeStatusWidget::BindCharacter(APawn* Pawn)
{
	UnBindCharacter();

	_BoundCharacter = Cast<ACharacterPlayer>(Pawn);
	if (true == _BoundCharacter.IsValid())
	{
		_BoundCharacter->_OnSelectedThrowableChanged.AddUniqueDynamic(this, &UGrenadeStatusWidget::UpdateGrenadeStatus);
	}
}

void UGrenadeStatusWidget::UnBindCharacter()
{
	if (true == _BoundCharacter.IsValid())
	{
		_BoundCharacter->_OnSelectedThrowableChanged.RemoveDynamic(this, &UGrenadeStatusWidget::UpdateGrenadeStatus);
	}

	_BoundCharacter.Reset();
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

	// 캐릭터가 고른 종류. 비어 있으면 인벤토리의 첫 투척물을 표시한다. (3번 키로 꺼내는 것과 같은 기준)
	FName SelectedTID = NAME_None;
	if (true == _BoundCharacter.IsValid())
	{
		SelectedTID = _BoundCharacter->GetSelectedThrowableTID();
	}

	// 투척물 종류별 수량 (같은 종류가 여러 칸이면 합산)
	TMap<FName, int32> CountByTID;
	FName FirstTID = NAME_None;

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

		CountByTID.FindOrAdd(Item._TID) += Item._Count;

		if (FirstTID.IsNone())
		{
			FirstTID = Item._TID;
		}
	}

	FName DisplayTID = FirstTID;
	if (CountByTID.Contains(SelectedTID))
	{
		DisplayTID = SelectedTID;
	}

	// 투척물이 없으면 숨긴다.
	if (DisplayTID.IsNone())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FItemData* DisplayRow = Table->FindTableRow<FItemData>(TEXT("ItemTable"), DisplayTID);
	if (nullptr != _IconImage && nullptr != DisplayRow && nullptr != DisplayRow->_Icon)
	{
		_IconImage->SetBrushFromTexture(DisplayRow->_Icon, true);

		// 같은 아이콘을 쓰는 투척물은 테이블의 색으로 구분 (색을 안 쓰면 원래 아이콘 색)
		FLinearColor IconColor = FLinearColor::White;
		if (true == DisplayRow->_bUseTint)
		{
			IconColor = DisplayRow->_TintColor;
		}
		_IconImage->SetColorAndOpacity(IconColor);
	}

	// 제목은 고른 종류의 이름 (아이템 테이블의 _DisplayName 그대로)
	if (nullptr != _HeadingText && nullptr != DisplayRow)
	{
		_HeadingText->SetText(DisplayRow->_DisplayName);
	}

	if (nullptr != _CountText)
	{
		_CountText->SetText(FText::AsNumber(CountByTID[DisplayTID]));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}
