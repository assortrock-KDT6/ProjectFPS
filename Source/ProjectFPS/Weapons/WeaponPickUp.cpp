// Fill out your copyright notice in the Description page of Project Settings.
#include "Weapons/WeaponPickUp.h"
#include "Weapons/WeaponTypes.h"
#include "Weapons/WeaponActor.h"
#include "Character/CharacterPlayer.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
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
	
	// 에디터에서는 기존 루트를 유지해 Blueprint에 저장된 배치가 바뀌지 않게 한다.
	_InteractionSphere->SetSphereRadius(25.0f);
	_InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_InteractionSphere->SetGenerateOverlapEvents(false);

	_PhysicsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupPhysicsBox"));
	_PhysicsBox->SetupAttachment(_InteractionSphere);
	_PhysicsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_PhysicsBox->SetGenerateOverlapEvents(false);

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

	// 무기 Mesh는 외형만 담당한다.
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

int AWeaponPickUp::GetCurrentAmmo() const
{
	return _CurrentAmmo;
}

void AWeaponPickUp::SetCurrentAmmo(int Ammo)
{
	_CurrentAmmo = Ammo;
}

// Called when the game starts or when spawned
void AWeaponPickUp::BeginPlay()
{
	Super::BeginPlay();

	// 구체로 시뮬레이션하면 무기가 공처럼 계속 구른다. 메시의 회전/크기를 반영한
	// 상자를 사용해 옆면이 바닥에 닿으면 안정적으로 멈추게 한다.
	const FTransform PlacementTransform = _InteractionSphere->GetComponentTransform();
	const FTransform MeshToPlacement = _Mesh->GetComponentTransform().GetRelativeTransform(PlacementTransform);
	const FBoxSphereBounds MeshBounds = _Mesh->CalcBounds(MeshToPlacement);
	const FVector BoxCenter = MeshBounds.Origin;
	const FVector BoxExtent = MeshBounds.BoxExtent.ComponentMax(FVector(1.0f));

	_InteractionSphere->SetSimulatePhysics(false);
	_InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_InteractionSphere->SetGenerateOverlapEvents(false);
	_Mesh->SetSimulatePhysics(false);
	_Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	_PhysicsBox->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SetRootComponent(_PhysicsBox);
	FTransform PhysicsTransform = PlacementTransform;
	if (HasAuthority())
	{
		PhysicsTransform.SetLocation(PlacementTransform.TransformPosition(BoxCenter));
	}
	// 클라이언트가 수신한 액터 위치는 이미 물리 상자 중심이므로 다시 이동하지 않는다.
	_PhysicsBox->SetWorldTransform(PhysicsTransform);
	_InteractionSphere->AttachToComponent(_PhysicsBox, FAttachmentTransformRules::KeepWorldTransform);
	_InteractionSphere->SetRelativeTransform(FTransform(-BoxCenter));

	_PhysicsBox->SetBoxExtent(BoxExtent);
	_PhysicsBox->SetMobility(EComponentMobility::Movable);
	_PhysicsBox->SetGenerateOverlapEvents(false);
	_PhysicsBox->SetCollisionObjectType(ECC_PhysicsBody);
	_PhysicsBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	_PhysicsBox->SetCollisionResponseToAllChannels(ECR_Block);
	_PhysicsBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	_PhysicsBox->SetLinearDamping(2.0f);
	_PhysicsBox->SetAngularDamping(5.0f);
	_PhysicsBox->SetPhysicsMaxAngularVelocityInDegrees(180.0f);
	_PhysicsBox->SetEnableGravity(true);
	_PhysicsBox->SetUseCCD(true);
	SetReplicateMovement(true);
	_PhysicsBox->SetSimulatePhysics(HasAuthority());
	if (HasAuthority())
	{
		ForceNetUpdate();
	}
	else
	{
		// 초기 복제가 BeginPlay보다 먼저 도착한 경우에도 새 루트에 위치/물리를 적용한다.
		SyncReplicatedPhysicsSimulation();
		OnRep_ReplicatedMovement();
	}
}

// Called every frame
void AWeaponPickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
