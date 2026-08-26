// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemPickUp.h"
#include "Components/StaticMeshComponent.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Common/GameDefines.h"
#include "Gamemode/PlayerStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"

AItemPickUp::AItemPickUp()
{
	_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(_Mesh);
}

void AItemPickUp::BeginPlay()
{
	Super::BeginPlay();
	
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;


	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), _TID);
	// 테이블 메시로 설정.
	if (Row && _Mesh && Row->_WorldMesh)
		_Mesh->SetStaticMesh(Row->_WorldMesh);
}

void AItemPickUp::Interact_Implementation(AActor* Interactor)
{
	APawn* Pawn = Cast<APawn>(Interactor);
	if (nullptr == Pawn)
		return;
	
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (nullptr == PC)
		return;

	APlayerStateBase* PS = PC->GetPlayerState<APlayerStateBase>();
	if (nullptr == PS)
		return;

	// 상호작용 추가 예정
	UInventoryComponent* Inv = PS->GetInventory();
	if (nullptr == Inv)
		return;
	
	// 테이블에서 타입 조회 
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	const FItemData* Row = Sub ? Sub->FindTableRow<FItemData>(TEXT("ItemTable"), _TID) : nullptr;

	// 타입 분기 : 무기 -> 장비슬롯, 그 외 아이템 슬롯
	if (Row && Row->_ItemType == EItemType::Weapon)
		Inv->EquipWeapon(_TID);						// 장비 슬롯
	else
		Inv->AddItem(_TID, _Count);					// 아이템 슬롯

	Destroy();					// 제거(PickUp)


}

void AItemPickUp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (nullptr == _ItemTable || _TID.IsNone())
		return;

	const FItemData* Row = _ItemTable->FindRow<FItemData>(_TID, TEXT(""));
	if (Row && _Mesh && Row->_WorldMesh)
		_Mesh->SetStaticMesh(Row->_WorldMesh);

}
