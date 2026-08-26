// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "ItemPickUp.generated.h"

UCLASS()
class PROJECTFPS_API AItemPickUp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AItemPickUp();

private:
	UPROPERTY(VisibleAnywhere, Category = "PickUp")
	TObjectPtr<class UStaticMeshComponent> _Mesh;

	// 아이템 (테이블 행)
	UPROPERTY(EditAnywhere, Category = "PickUp")
	FName _TID;

	//BP에서 DT_ItemTable 지정
	UPROPERTY(EditDefaultsOnly, Category = "PickUp")
	TObjectPtr<UDataTable> _ItemTable;

	UPROPERTY(EditAnywhere, Category = "PickUp")
	int32 _Count = 1;

public:
	virtual void BeginPlay() override;
	virtual void Interact_Implementation(AActor* Interactor) override; // 인터페이스 구현 -> void -> bool
	virtual void OnConstruction(const FTransform& Transform) override;



};
