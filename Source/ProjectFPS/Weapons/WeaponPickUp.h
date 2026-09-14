// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "WeaponPickUp.generated.h"

class USphereComponent;
UCLASS()
class PROJECTFPS_API AWeaponPickUp : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponPickUp();

	// F키 상호작용시 해당 무기를 플레이어에게 장착
	virtual void Interact_Implementation(AActor* Interactor) override;
	
	// 에디터에서 ItemId에 해당하는 월드 Mesh를 미리 표시한다
	virtual void OnConstruction(const FTransform& Transform) override;
	
protected:
	// 플레이어가 무기의 상호작용할 수 있는 범위 콜라이더 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon PickUp")
	TObjectPtr<USphereComponent> _InteractionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon PickUp")
	TObjectPtr<UStaticMeshComponent> _StaticMesh;
	
	// ItemDataTable에서 찾을 무기 아이템의 행 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon PickUp")
	FName _ItemId = NAME_None;
	
	// 에디터 미리보기용 -> BP_WeaponPickUp 기본값에 한번만 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon PickUp")
	TObjectPtr<UDataTable> _ItemTable;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
