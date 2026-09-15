// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "ItemPickUp.generated.h"

// 인터페이스에서 상속되서 아이템에 대한 정보를 테이블을 통해서 판별하고
// 인벤토리에 구분해서 넣고 월드 상에 픽업 대상이된 아이템을 삭제처리까지.


UCLASS()
class PROJECTFPS_API AItemPickUp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AItemPickUp();

protected:
	UPROPERTY(VisibleAnywhere, Category = "ItemInfo")
	TObjectPtr<class UStaticMeshComponent> _Mesh;
	
	// 아이템 (테이블 행)
	UPROPERTY(EditAnywhere, Category = "ItemInfo")
	FName _TID;

	//BP에서 DT_ItemTable 지정
	UPROPERTY(EditDefaultsOnly, Category = "ItemInfo")
	TObjectPtr<UDataTable> _ItemTable;

	UPROPERTY(EditAnywhere, Category = "ItemInfo")
	int32 _Count = 1;
	
public:
	// 조준 받은 아이템의 외곽선 강조.
	/*void SetHightlight(bool bOn);*/ // 나중에 쉐이더로 처리할게요.

	// 픽업이 가지고 있는 정보를 외부로 넘기기 위함. 
	FName GetTID() const { return _TID; }

public:
	virtual void BeginPlay() override;
	virtual void Interact_Implementation(AActor* Interactor) override; //  void -> bool
	virtual void OnConstruction(const FTransform& Transform) override;



};
