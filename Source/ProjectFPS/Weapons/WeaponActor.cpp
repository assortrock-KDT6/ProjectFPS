// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/WeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Table/TableSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameMode/FPSGameMode.h"
#include "GameMode/PlayerStateBase.h"

// Sets default values
AWeaponActor::AWeaponActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	_WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(_WeaponMesh);
	
	// 소유자는 별도의 SkeletalMesh 총기를 본다. 
	_WeaponMesh->SetOwnerNoSee(true);
	
	// 장착된 총기 자체가 캐릭터나 탄환과 충돌하지 않게 하기
	_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_WeaponMesh->SetGenerateOverlapEvents(false);
	
	// 멀티플레이 복제
	bReplicates = true;
	SetReplicateMovement(true);
}

bool AWeaponActor::InitializeWeapon_Implementation(FName WeaponID)
{
	_WeaponID = WeaponID;

	return !_WeaponID.IsNone() && LoadWeaponData(_WeaponID);
}

void AWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponActor, _WeaponID);
	DOREPLIFETIME(AWeaponActor, _CurrentFireMode);
}

FVector AWeaponActor::GetMuzzleLocation() const
{
	return _WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
}

float AWeaponActor::GetWeaponRange() const
{
	return _WeaponAbilityData._Range;
}

const FWeaponData& AWeaponActor::GetWeaponData() const
{
	return _WeaponData;
}

EWeaponFireMode AWeaponActor::GetFireMode() const
{
	return _CurrentFireMode;
}

float AWeaponActor::GetProjectileInterval() const
{
	return _WeaponAbilityData._ProjectileInterval;
}

bool AWeaponActor::SupportsFireMode(EWeaponFireMode Mode) const
{
	return Mode != EWeaponFireMode::None && (Mode == _WeaponAbilityData.FireMode
		|| _WeaponAbilityData.SupportedFireModes.Contains(Mode));
}

bool AWeaponActor::CanToggleFireMode() const
{
	return SupportsFireMode(EWeaponFireMode::SemiAutomatic) && SupportsFireMode(EWeaponFireMode::Automatic);
}

bool AWeaponActor::ToggleFireMode()
{
	if (false == HasAuthority() || false == CanToggleFireMode())
	{
		return false;
	}

	_CurrentFireMode = _CurrentFireMode == EWeaponFireMode::Automatic ? EWeaponFireMode::SemiAutomatic : EWeaponFireMode::Automatic;
	
	ForceNetUpdate();
	
	return true;
}

double AWeaponActor::GetRemainingFireInterval() const
{
	return GetWorld() ? FMath::Max(0., _NextAllowedShotTime - GetWorld()->GetTimeSeconds()) : 0.;
}

bool AWeaponActor::Fire(const FVector& AimPoint)
{
	if (nullptr == GetWorld() || false == HasAuthority() || GetRemainingFireInterval() > 0.
		|| false == SupportsFireMode(_CurrentFireMode) || GetProjectileInterval() <= 0.f)
	{
		return false;
	}

	const AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();

	const UFPSAbilitySystemComponent* AbilitySystem = Cast<UFPSAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator()));

	if (false == HasAuthority() || false == IsValid(GameMode) || false == GameMode->IsCombatAllowed()
		|| false == IsValid(AbilitySystem) || false == AbilitySystem->CanAttack())
	{
		return false;
	}
	
	if (false == IsValid(GetWorld()) 
		|| false == IsValid(_WeaponMesh) 
		|| nullptr == _ProjectileClass 
		|| false == _WeaponMesh->DoesSocketExist(TEXT("Muzzle")))
	{
		return false;
	}
	
	const FVector MuzzleLocation = GetMuzzleLocation();
	
	// 카메라 Trace 로 구한 AimPoint를 향하도록 총구 기준 발사 방향을 계산
	const FVector FireDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	
	if (FireDirection.IsNearlyZero())
	{
		return false;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.bDeferConstruction = true;
	
	AActor* Projectile = GetWorld()->SpawnActor<AActor>(_ProjectileClass, MuzzleLocation, FireDirection.Rotation(), SpawnParameters);
	if (!IsValid(Projectile))
	{
		return false;
	}

	// Blueprint의 Overlap/BeginPlay보다 먼저 발사 시점의 소유자를 기록한다.
	UDamageSourceComponent* DamageSource = NewObject<UDamageSourceComponent>(Projectile);
	Projectile->AddInstanceComponent(DamageSource);
	DamageSource->SetSourcePlayerState(IsValid(GetInstigator()) ? GetInstigator()->GetPlayerState<APlayerStateBase>() : nullptr);
	DamageSource->InitializeProjectileDamage(_WeaponAbilityData._Damage);
	DamageSource->RegisterComponent();
	// Reserve before BeginPlay/overlap callbacks can attempt another shot.
	_NextAllowedShotTime = GetWorld()->GetTimeSeconds() + GetProjectileInterval();
	Projectile->FinishSpawning(FTransform(FireDirection.Rotation(), MuzzleLocation));
	
	return IsValid(Projectile);
}

void AWeaponActor::OnRep_WeaponID()
{
	LoadWeaponData(_WeaponID);
}

bool AWeaponActor::LoadWeaponData(FName WeaponID)
{
	UTableSubsystem* TableSubsystem = UTableSubsystem::Get(this);
	if (false == IsValid(TableSubsystem))
	{
		return false;
	}

	const FWeaponData* WeaponData = TableSubsystem->FindTableRow<FWeaponData>(TEXT("WeaponDataTable"), WeaponID);

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
	// 반동 데이터의 Key 와 실제 조회에 사용한 Row Name을 일치시킨다
	_WeaponData._WeaponId = WeaponID;
	_WeaponAbilityData = *WeaponAbilityData;
	if (HasAuthority())
	{
		_CurrentFireMode = _WeaponAbilityData.FireMode;
	}
	_WeaponMesh->SetStaticMesh(_WeaponData._StaticMesh);

	return _WeaponData.IsValid();
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

