// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/WeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Table/TableSubsystem.h"

// Sets default values
AWeaponActor::AWeaponActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	_WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(_WeaponMesh);
}

bool AWeaponActor::InitializeWeapon_Implementation(FName _WeaponID)
{
	UTableSubsystem* TableSubsystem = UTableSubsystem::Get(this);
	if (false == IsValid(TableSubsystem))
	{
		return false;
	}
	
	const FWeaponData* WeaponData = TableSubsystem->FindTableRow<FWeaponData>(TEXT("WeaponDataTable"), _WeaponID);
	
	if (nullptr == WeaponData)
	{
		return false;
	}
	
	const FWeaponAbilityDataTable* WeaponAbilityData = TableSubsystem->FindTableRow<FWeaponAbilityDataTable>(TEXT("WeaponAbilityDataTable"), WeaponData->_WeaponAbilId);
	if (nullptr == WeaponAbilityData)
	{
		return false;
	}
	
	// 테이블 조회가 모두 성공한 뒤 무기 액터 내부에 복사
	_WeaponData = *WeaponData;
	_WeaponAbilityData = *WeaponAbilityData;
	_WeaponMesh->SetStaticMesh(_WeaponData._StaticMesh);
	
	return IsValid(_WeaponData._StaticMesh);
}

FVector AWeaponActor::GetMuzzleLocation() const
{
	return _WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
}

// Called when the game starts or when spawned
void AWeaponActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

