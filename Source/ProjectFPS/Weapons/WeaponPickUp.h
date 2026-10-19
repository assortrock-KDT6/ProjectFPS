// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemPickUp.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "WeaponPickUp.generated.h"

class USphereComponent;
class UBoxComponent;
class UPrimitiveComponent;
struct FWeaponData;

// AItemPickUp을 상속 -> 무기 픽업 시 손에 장착 기능까지.


UCLASS()
class PROJECTFPS_API AWeaponPickUp : public AItemPickUp
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponPickUp();

	// F키 상호작용시 해당 무기를 플레이어에게 장착
	virtual void Interact_Implementation(AActor* Interactor) override;
	
	// 에디터에서 ItemId에 해당하는 월드 Mesh를 미리 표시한다
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	int		GetCurrentAmmo() const;
	void	SetCurrentAmmo(int Ammo);

protected:
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
	
protected:
	// 기존 Blueprint의 배치와 자식 컴포넌트 위치를 유지하는 기준점.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon PickUp")
	TObjectPtr<USphereComponent> _InteractionSphere;

	// 실행 시 무기 크기에 맞춰 물리 루트로 사용한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon PickUp")
	TObjectPtr<UBoxComponent> _PhysicsBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon PickUp")
	TObjectPtr<USphereComponent> _PickupSphere;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon PickUp | Data")
	int	_CurrentAmmo;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//// 부모 줍기 순서에 끼어드는 훅.
	//virtual bool PrepareAcquire(ACharacterPlayer* Character, const FItemData& Row) override;
	//virtual bool CommitAcquire(ACharacterPlayer* Character) override;
	//virtual void CancelAcquire(ACharacterPlayer* Character) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
