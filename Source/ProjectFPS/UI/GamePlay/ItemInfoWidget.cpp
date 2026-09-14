// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/ItemInfoWidget.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Weapons/WeaponTypes.h"
#include "Common/GameDefines.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"




void UItemInfoWidget::SetWeaponAbility(FName TID)
{
	// 테이블에서 무기 능력치 가져오기
	if (nullptr == _AbilityText)
		return;

	// 테이블 조회 시스템 가져오기
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
	{
		HideAbility();
		return;
	}

	// ItemTable 과 같은 TID로 무기 테이블 조회.
	const FWeaponData* Weapon = Sub->FindTableRow<FWeaponData>(TEXT("WeaponTable"), TID);	// WeaponTable 에서 TID에 해당하는 정보 찾기.
	if (nullptr == Weapon)
	{
		HideAbility();
		return;
	}

	// 무기 행에 적힌 ID로 능력치 찾기 
	const FWeaponAbilityDataTable* Abil = Sub->FindTableRow<FWeaponAbilityDataTable>(TEXT("WeaponAbilityDataTable"), Weapon->_WeaponAbilId);
	if (nullptr == Abil)
	{
		HideAbility();
		return;
	}
	
	FString Ability;


	
}

void UItemInfoWidget::HideAbility()
{
	// Collapsed : 위젯을 숨기면서 배치할떄 공간도 차지하지 않게함 -> Hidden은 공백으로보이고, 이건 공백이 땡겨짐.
	if (nullptr != _AbilityText)
		_AbilityText->SetVisibility(ESlateVisibility::Collapsed);
}

void UItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 시작은 숨김 처리.
	HideInfo();	
}

void UItemInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// 매 프레임마다 처리 및 갱신. 
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 숨겨진 상태이면 마우스에서 x 
	if (ESlateVisibility::Hidden == GetVisibility())
		return;

	// 이 위젯이 캔버스 패널에 놓여 있어야 위치를 바꿀 수 있음.
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
	if (nullptr == CanvasSlot)
		return;

	// 뷰포트 기준 마우스 좌표(DPI 스케일 반영됨)
	const FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
	// 정보창 위치 변경.
	CanvasSlot->SetPosition(MousePos + _CursorOffset);


}

void UItemInfoWidget::SetInfoByTID(FName TID)
{
	// 받아 올 행이 없을 경우 숨김처리
	if (TID.IsNone())
	{
		HideInfo();
		return;
	}

	// 테이블 조회용 시스템 가져오기.
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;

	// TID(ItemTable) 해당하는 행 찾기.
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);
	
	// 정보 값을 못 찾을 경우 숨기고 종료.
	if (nullptr == Row)
	{
		HideInfo();
		return;
	}

	// 이름
	if (nullptr != _NameText)
		_NameText->SetText(Row->_DisplayName);

	// 설명
	if (nullptr != _DescText)
		_DescText->SetText(Row->_Description);

	// 아이콘
	if (nullptr != _IconImage)
	{
		if (nullptr != Row->_Icon)
		{
			_IconImage->SetBrushFromTexture(Row->_Icon);
			_IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			_IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UItemInfoWidget::HideInfo()
{
	// *걍 삭제를 할까?
	// ESlateVisibility -> 위젯 표시 상태를 모아 놓은 enum 
	SetVisibility(ESlateVisibility::Hidden);
}
