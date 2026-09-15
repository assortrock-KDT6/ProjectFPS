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
	bReplicates = true;

}

//void AItemPickUp::SetHightlight(bool bOn)
//{
//	if (nullptr == _Mesh)
//		return;
//	_Mesh->SetRenderCustomDepth(bOn);
//}

void AItemPickUp::BeginPlay()
{
	Super::BeginPlay();
	
	// 테이블 관리 시스템을 가져온다.
	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;

	// 관리 시스템에서 ItemTable을 찾아 픽업의 _ItemTable 변수에 저장합니다.
	_ItemTable = Sub->FindTable(TEXT("ItemTable"));
	if (nullptr == _ItemTable)
		return;
	
	// 저장해둔 테이블에서 자신의_TID에 해당하는 행을 찾는다.
	const FItemData* Row = _ItemTable->FindRow<FItemData>(_TID,TEXT("not Found row"));

	// 행 메시 컴포넌트 테이블의 메시가 모두 있으면 픽업의 외형으로 해당 메시로 설정함.
	if (Row && _Mesh && Row->_WorldMesh)
		_Mesh->SetStaticMesh(Row->_WorldMesh);
}

void AItemPickUp::Interact_Implementation(AActor* Interactor)
{
	// 상호작용한 캐릭터의 인벤토리를 찾아 아이템 추가 요청을 하고 월드에 아이템을 제거함.
	// 여기서 제거를 하는거에 대해서는 고민? 


	// 서버에서만 체크
	if (false == HasAuthority())
		return;

	// 상호작용한 액터를 Pawn으로 확인.
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
	// 인벤토리까지 찾아가기 
	UInventoryComponent* Inv = PS->GetInventory();
	if (nullptr == Inv)
		return;

	// 인벤토리에 요청하고 결과에만 반응 
	if (Inv->TryAquire(_TID, _Count))
		// (자기자신) 해당하는 액터는 제거 -> 월드상에 아이템
		Destroy();
	
	
}

void AItemPickUp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 테이블이 비어있으면 반환
	if (nullptr == _ItemTable || _TID.IsNone())
		return;

	// 테이블 행을으로 메시정보 받아오기.
	const FItemData* Row = _ItemTable->FindRow<FItemData>(_TID, TEXT(""));
	if (Row && _Mesh && Row->_WorldMesh)
		_Mesh->SetStaticMesh(Row->_WorldMesh);

}
