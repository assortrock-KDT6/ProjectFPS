// Fill out your copyright notice in the Description page of Project Settings.
#include "Weapons/WeaponPickUp.h"
#include "Character/CharacterPlayer.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Table/TableDatas.h"
#include "Table/TableSubsystem.h"

// Sets default values
AWeaponPickUp::AWeaponPickUp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	_InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(_InteractionSphere);

	// 플레이어만 상호작용 범위에 들어왔는지 감지한다.
	_InteractionSphere->SetSphereRadius(200.0f);
	_InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	_InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	_InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	_InteractionSphere->SetGenerateOverlapEvents(true);

	_StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	_StaticMesh->SetupAttachment(_InteractionSphere);

	// 무기 Mesh는 외형만 담당하고 상호작용 판정은 Sphere가 담당한다.
	_StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponPickUp::Interact_Implementation(AActor* Interactor)
{
	if (false == HasAuthority())
	{
		return;
	}
	
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(Interactor);
	if (false == IsValid(Character))
	{
		return;
	}
	
	UTableSubsystem* TableSubsystem = UTableSubsystem::Get(this);
	if (false == IsValid(TableSubsystem))
	{
		return;
	}
	
	const FItemData* ItemData = TableSubsystem->FindTableRow<FItemData>(TEXT("ItemTable"), _ItemId);
	
	if (nullptr == ItemData || EItemType::Weapon != ItemData->_ItemType || ItemData->_WeaponId.IsNone())
	{
		return;
	}
	
	if (Character->EquipWeapon(ItemData->_WeaponId))
	{
		Destroy();
	}
}

void AWeaponPickUp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (false == IsValid(_StaticMesh))
	{
		return;
	}
	
	// ItemId가 바뀌었을때 이전 Mesh가 남지 않도록 먼저 비운다.
	_StaticMesh->SetStaticMesh(nullptr);
	
	if (false == IsValid(_ItemTable) || _ItemId.IsNone())
	{
		return;
	}
	
	const FItemData* ItemData = _ItemTable->FindRow<FItemData>(_ItemId, TEXT("WeaponPickUp"));
	
	if (nullptr == ItemData || EItemType::Weapon != ItemData->_ItemType)
	{
		return;
	}
	
	_StaticMesh->SetStaticMesh(ItemData->_WorldMesh);
}

// Called when the game starts or when spawned
void AWeaponPickUp::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponPickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

