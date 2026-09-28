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
#include "Character/CharacterPlayer.h"
#include "Net/UnrealNetwork.h"


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

AItemPickUp* AItemPickUp::BeginSpawnFromTID(UWorld* world, FName TID, int32 Count, const FTransform& Transform)
{
	// 월드상에 아이템을 생성시킴. -> 버리기 기능을 위함.

	// 서버 전용 
	// 월드가 없가나 클라이언트의 경우 종료함.
	if (nullptr == world || world->IsNetMode(NM_Client))
		return nullptr;

	// 아이템을 식별할 TID가 없거나 0이하면 생성하지 않음.
	if (TID.IsNone() || Count <= 0)
		return nullptr;

	// 테이블 조회 시스템 가져오기.
	UTableSubsystem* Sub = UTableSubsystem::Get(world);
	// 예외처리.
	if (nullptr == Sub)
		return nullptr;

	// TID로 아이템 찾기.
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);
	if (nullptr == Row)
	{
		// 디버그 로그확인용.
		UE_LOG(LogTemp, Error, TEXT("[ITemPickUp] ItemTable에 '%s' 행 없음"), *TID.ToString());
		return nullptr;
	}

	// 생성할 픽업 클래스 확인.
	if (nullptr == Row->_PickUpClass)
	{
		// 디버그 로그확인용.
		UE_LOG(LogTemp, Error, TEXT("[ITemPickUp] ItemTable['%s']에 _PickUpclass 없음"), *TID.ToString());

		return nullptr;
	}

	// 픽업 액터 생성
	AItemPickUp* Pickup = world->SpawnActorDeferred <AItemPickUp>( // 생성 완료 전에 정보를 넣기 위함.
		Row->_PickUpClass,	// 실제 생성할 클래스 
		Transform,			// 생설할 위치
		nullptr,			// (Owner) x 
		nullptr,			// (instigator) 폰 지정.
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);	// 충돌시 위치 조정을 시도 실패해도 생성.

	if (nullptr == Pickup)
	{
		// 디버그 로그확인용.
		UE_LOG(LogTemp, Error, TEXT("[ItemPickUp] '%s' 스폰 실패"), *TID.ToString());
		return nullptr;
	}
	
	// 픽업에 정보 넣기 -> 월드에 생성될 때 필요한 정보들. (현재는 버리기 기능기준 버려질 때.)
	Pickup->SetTID(TID);
	Pickup->SetCount(Count);

	// 준비 상태 -> 확정 전까지 보이지도 부딪히지도 않게.
	Pickup->SetActorHiddenInGame(true);
	Pickup->SetActorEnableCollision(false);
	return Pickup;

}

void AItemPickUp::FinishSpawnFromTID(AItemPickUp* Pickup, const FTransform& Transform)
{
	if (false == IsValid(Pickup))
		return;
	// 생성 완료 후에 해제 
	Pickup->FinishSpawning(Transform);
	if (false == IsValid(Pickup))
		return;

	// 준비 상태 -> 확정 전까지 보이지도 부딪히지도 않게.
	Pickup->SetActorHiddenInGame(false);
	Pickup->SetActorEnableCollision(true);
}

void AItemPickUp::BeginPlay()
{
	Super::BeginPlay();
	RefreshMeshFromTable();
}

void AItemPickUp::Interact_Implementation(AActor* Interactor)
{
	// 상호작용한 캐릭터의 인벤토리를 찾아 아이템 추가 요청을 하고 월드에 아이템을 제거함.
	UE_LOG(LogTemp, Warning, TEXT("[Interact] %s TID=%s Auth=%d"), *GetClass()->GetName(), *_TID.ToString(), HasAuthority());

	// 서버에서만 체크
	if (false == HasAuthority())
		return;

	// 상호작용한 액터 -> Pc -> Ps -> 인벤
	APawn* Pawn = Cast<APawn>(Interactor);
	if (nullptr == Pawn)
		return;
	
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (nullptr == PC)
		return;

	APlayerStateBase* PS = PC->GetPlayerState<APlayerStateBase>();
	if (nullptr == PS)
		return;
	
	// 인벤토리까지 찾아가기 
	UInventoryComponent* Inv = PS->GetInventory();
	if (nullptr == Inv)
		return;

	// 테이블 행
	UTableSubsystem* Sub= UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return;
	
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), _TID);
	if (nullptr == Row)
		return;
	// 소모품 : 인벤 반영 성공시 삭제 (종류별 추가 및 수정.)
	if (EItemType::Weapon != Row->_ItemType)
	{
		if (Inv->TryAcquire(_TID, _Count))
			Destroy();
		return;
	}

	// 무기
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(Interactor);
	if (nullptr == Character)
		return;

	// 슬롯찾기.
	int32 SlotIndex = INDEX_NONE;
	FName ReplacedTID = NAME_None;
	if (false == Inv->FindEquipSlot(_TID, SlotIndex, ReplacedTID))
		return;


	// 밀려날 무기가 있으면 이 자리에 픽업 준비 (Deferred — 아직 월드에 없음)
	AItemPickUp* DropPickUp = nullptr;
	const FTransform DropTransform = GetActorTransform();
	if (false == ReplacedTID.IsNone())
	{
		DropPickUp = BeginSpawnFromTID(GetWorld(), ReplacedTID, 1, DropTransform);
		if (nullptr == DropPickUp)
		{
			UE_LOG(LogTemp, Error, TEXT("[ItemPickUp] '%s' 밀려날 무기 픽업 생성 실패 — 테이블 _PickUpClass 확인"), *ReplacedTID.ToString());
			return;
		}
	}

	// 인벤 확정 — 실패 시 준비한 픽업만 정리
	if (false == Inv->CommitEquip(_TID, SlotIndex, false))
	{
		if (IsValid(DropPickUp))
			DropPickUp->Destroy();
		return;
	}

	// 마무리: 밀려난 픽업 등장 → 자기 삭제 → 알림
	if (IsValid(DropPickUp))
	{
		AItemPickUp::FinishSpawnFromTID(DropPickUp, DropTransform);
	}
	Destroy();
	Inv->NotifyWeaponsChanged();
	

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

void AItemPickUp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// 부모 클래스의 복제 등록도 유기하기 위해 호출
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 서버가 가진 아이템TID와수량을 클라에게 전달할 대상으로 등록.
	DOREPLIFETIME(AItemPickUp, _TID);
	DOREPLIFETIME(AItemPickUp, _Count);
}

void AItemPickUp::OnRep_TID()
{
	RefreshMeshFromTable();
}

void AItemPickUp::RefreshMeshFromTable()
{
	if (nullptr == _Mesh)
		return;

	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
	{
		_ItemTable = nullptr;
		_Mesh->SetStaticMesh(nullptr);
		return;
	}

	_ItemTable = Sub->FindTable(TEXT("ItemTable"));
	if (nullptr == _ItemTable || _TID.IsNone())
	{
		_Mesh->SetStaticMesh(nullptr);
		return;
	}

	const FItemData* Row = _ItemTable->FindRow<FItemData>(_TID, TEXT("RefreshMeshFromTable"));
	if(nullptr == Row)
	{
		_Mesh->SetStaticMesh(nullptr);
		return;
	}

	// 메시가 nullptr이면 기존 메시도 비워짐.
	_Mesh->SetStaticMesh(Row->_WorldMesh);
}
