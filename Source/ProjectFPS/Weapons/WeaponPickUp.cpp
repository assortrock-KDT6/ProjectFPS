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
	SetReplicateMovement(true);
	
	_InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(_InteractionSphere);
	
	// 기존 루트 컴포넌트를 물리 충돌체로 유지해 배치된 Blueprint의 루트/위치를 보존한다.
	_InteractionSphere->SetSphereRadius(25.0f);
	_InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	_InteractionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	_InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	_InteractionSphere->SetGenerateOverlapEvents(false);

	// 플레이어 감지 범위는 물리 루트를 따라 움직이되 물리 충돌에는 참여하지 않는다.
	_PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	_PickupSphere->SetupAttachment(_InteractionSphere);
	_PickupSphere->SetSphereRadius(70.0f);
	_PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	_PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	_PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	_PickupSphere->SetGenerateOverlapEvents(true);
	_PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickUp::OnInteractionSphereBeginOverlap);
	_PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponPickUp::OnInteractionSphereEndOverlap);
	
	_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	_Mesh->SetupAttachment(_InteractionSphere);

	// 무기 Mesh는 외형만 담당하고 상호작용 판정은 Sphere가 담당한다.
	_Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponPickUp::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(OtherActor);

	if (false == IsValid(Character))
	{
		return;
	}
	
	Character->SetNearbyWeaponPickUp(this);
}

void AWeaponPickUp::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(OtherActor);
	if (false == IsValid(Character))
	{
		return;
	}
	
	Character->ClearNearbyWeaponPickUp(this);
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
	
	const FItemData* ItemData = TableSubsystem->FindTableRow<FItemData>(TEXT("ItemTable"), _TID);
	
	if (nullptr == ItemData || EItemType::Weapon != ItemData->_ItemType || ItemData->_WeaponId.IsNone())
	{
		return;
	}
	
	Character->EquipWeapon(ItemData->_WeaponId);

	Super::Interact_Implementation(Interactor);
}

void AWeaponPickUp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (false == IsValid(_Mesh))
	{
		return;
	}
	
	// ItemId가 바뀌었을때 이전 Mesh가 남지 않도록 먼저 비운다.
	_Mesh->SetStaticMesh(nullptr);
	
	if (false == IsValid(_ItemTable) || _TID.IsNone())
	{
		return;
	}
	
	const FItemData* ItemData = _ItemTable->FindRow<FItemData>(_TID, TEXT("WeaponPickUp"));
	
	if (nullptr == ItemData || EItemType::Weapon != ItemData->_ItemType)
	{
		return;
	}
	
	_Mesh->SetStaticMesh(ItemData->_WorldMesh);
}

// Called when the game starts or when spawned
void AWeaponPickUp::BeginPlay()
{
	Super::BeginPlay();

	// 기존 Blueprint의 루트 구체 충돌/축 잠금 오버라이드를 런타임에 바로잡는다.
	_InteractionSphere->SetSphereRadius(25.0f);
	_InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	_InteractionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	_InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	_InteractionSphere->SetGenerateOverlapEvents(false);
	_InteractionSphere->BodyInstance.bLockTranslation = false;
	_InteractionSphere->BodyInstance.bLockRotation = false;
	_InteractionSphere->SetEnableGravity(true);
	_InteractionSphere->SetSimulatePhysics(HasAuthority());
}

// Called every frame
void AWeaponPickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
