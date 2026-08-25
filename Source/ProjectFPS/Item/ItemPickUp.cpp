// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemPickUp.h"
#include "Components/StaticMeshComponent.h"
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

void AItemPickUp::Interact(AActor* Interactor)
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
	PS->AddItem(_TID, _Count);	// 보유에 추가
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
