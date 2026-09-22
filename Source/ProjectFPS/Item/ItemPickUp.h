// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "ItemPickUp.generated.h"

// 인터페이스에서 상속되서 아이템에 대한 정보를 테이블을 통해서 판별하고
// 인벤토리에 구분해서 넣고 월드 상에 픽업 대상이된 아이템을 삭제처리까지.
// 월드에 놓인 아이템 픽업 액터의 공통 베이스 

//class ACharacterPlayer;
class UDataTable;
struct FItemData;

// 모든 픽업의 공통 부모.
// 줍기 순서 : (찾기 -> 준비 -> 확정 -> 마무리) 여기서만 관리.
// 자식은 훅 3개로 끼워넣음.

UCLASS(Abstract)	// 이 클래스 자체로 객체를 만들지 않고, 자식 클래스의 부모로 사용하겠다 선언.
class PROJECTFPS_API AItemPickUp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AItemPickUp();

protected:
	UPROPERTY(VisibleAnywhere, Category = "ItemInfo")
	TObjectPtr<class UStaticMeshComponent> _Mesh;
	
	// 아이템 (테이블 행)
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TID, Category = "ItemInfo")
	FName _TID;

	//BP에서 DT_ItemTable 지정
	UPROPERTY(EditDefaultsOnly, Category = "ItemInfo")
	TObjectPtr<UDataTable> _ItemTable;

	// 서버가 지정한 수량을 클라이언트에도 전달함.
	UPROPERTY(EditAnywhere, Replicated, Category = "ItemInfo")
	int32 _Count = 1;
	
public:
	// 조준 받은 아이템의 외곽선 강조.
	/*void SetHightlight(bool bOn);*/ // 나중에 쉐이더로 처리할게요.

	// 픽업이 가지고 있는 정보를 외부로 넘기기 위함. 
	FName GetTID() const { return _TID; }

	// 스폰 직후(FindishSpawnFromTID 전) 값 지정용.
	void SetTID(FName TID) { _TID = TID; }
	void SetCount(int32 Count) { _Count = Count; }

	// TID로 픽ㄱ업을 Deferred 상태로 만든다. 클래스는 ItemTable, _PickUpClass가 정함.
	// 준비 중엔 숨김,충돌 꺼짐. 성공 유무에 따라 FinishSpawnFromTID,Destroy 구분 (서버)
	static AItemPickUp* BeginSpawnFromTID(UWorld* world, FName TID, int32 Count, const FTransform& Transform);

	// BeginSawpnFromTID로 준비한 픽업을 실제로 등장 시킴(생성완료 -> 숨김 충돌 해제) 실제 구현
	static void FinishSpawnFromTID(AItemPickUp* Pickup, const FTransform& Transform);


public:
	virtual void BeginPlay() override;
	virtual void Interact_Implementation(AActor* Interactor) override; 
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_TID();

	// 테이블에서 _TID 행의 메시를 붙인다. 행/메시가 없으면 비운다.
	void RefreshMeshFromTable();

//protected:
//	// 자식 줍기 전 훅
//	// 이벤 화정 전. 자식이 자기 준비 -> 실패시 부모가 전체 취소.
//	// 기본 false 장비류인데 훅을 안채운 자식은 줍기를 아예 막아버림(오류방지)
//	virtual bool PrepareAcquire(ACharacterPlayer* Character, const FItemData& Row) { return false; }
//
//	// 인벤 확정 후 자식이 준비한 것을 확정 -> 시랲시 false 부모가 인벤 롤백.
//	// 실패 판정은 기존 상태 업데이트 전에 끝냄.
//	virtual bool CommitAcquire(ACharacterPlayer* Character) { return false; }
//
//	// 도중 실패 자식이 준비한것을 정리 여러 번 호출시에도 안전하게.
//	virtual void CancelAcquire(ACharacterPlayer* Character) {}

};
