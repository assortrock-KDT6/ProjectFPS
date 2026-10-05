// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/WeaponActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Table/TableSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
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
	DOREPLIFETIME(AWeaponActor, _CurrentAmmo);
	DOREPLIFETIME(AWeaponActor, _CurrentFireMode);
}

// Commit 건영 : 아래 Line 145에서 이유 서술 ( 필요없는 분할이라서 호출위치 한곳에서 깔끔하게 관리하려고 지워요 ) 
// FVector AWeaponActor::GetMuzzleLocation() const
// {
// 	return _WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
// }

float AWeaponActor::GetWeaponRange() const
{
	return _WeaponAbilityData._Range;
}

const FWeaponData& AWeaponActor::GetWeaponData() const
{
	return _WeaponData;
}

USceneComponent* AWeaponActor::GetWeaponMeshComponent() const
{

	return _WeaponMesh;
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
	// Commit 건영 : 중복 검사 통합
	// HasAuthority 를 두번 검사하고 World도 앞에서 nullptr, 뒤에서 IsValid로 두번 검사해서 수정함
	if (false == IsValid(GetWorld()) ||
		false == HasAuthority() || 
		GetRemainingFireInterval() > 0. || 
		false == SupportsFireMode(_CurrentFireMode) || 
		GetProjectileInterval() <= 0.f)
	{
		return false;
	}

	// Commit 건영 : 아래 CanAttack에서 서버의 GameMode 조회와 경기 상태 검사를 수행하고 있어서 같은 검사하려고 GameMode 변수 만드는건 불필요
	// const AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();

	const UFPSAbilitySystemComponent* AbilitySystem = Cast<UFPSAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator()));

	// Commit 건영 : 중복코드이고 FPSAbilitySystemComponent 랑 CanAttack에서 중복
	// if (false == HasAuthority() || false == IsValid(GameMode) || false == GameMode->IsCombatAllowed()
	// 	|| false == IsValid(AbilitySystem) || false == AbilitySystem->CanAttack())
	// {
	// 	return false;
	// }
	
	if (false == IsValid(AbilitySystem) || false == AbilitySystem->CanAttack())
	{
		return false;
	}
	
	// Commit 건영 :  IsValid(GetWorld()) 위로 올림 
	if (false   == IsValid(_WeaponMesh) || 
		nullptr == _ProjectileClass        || 
		false   == _WeaponMesh->DoesSocketExist(TEXT("Muzzle")))
	{
		return false;
	}
	
	// Commit 건영 : 불필요 분할 함수
	// line 51~54 (FVector AWeaponActor::GetMuzzleLocation() const {...}) 에서 소캣 위치 조회 한줄만 실행
	// 현재 C++ 호출도 여기뿐이고 함수 내부에 별도의 검증이나 변환이 없으니까 조회를 사용하는 위치에 모으고 기존 MuzzleLocation 변수를 그대로 쓰면서 위 함수 정리
	//const FVector MuzzleLocation = GetMuzzleLocation();
	const FVector MuzzleLocation = _WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
	
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
	SubCurrentAmmo(1);
	
	return IsValid(Projectile);
}

void AWeaponActor::SubCurrentAmmo(int NewAmmo)
{
	const int32 PreviousAmmo = _CurrentAmmo;

	if (0 > _CurrentAmmo - NewAmmo)
	{
		_CurrentAmmo = 0;
	}
	else
	{
		_CurrentAmmo -= NewAmmo;
	}

	// 서버에서 직접 변경한 값도 리슨 서버의 UI에 알린다.
	if (PreviousAmmo != _CurrentAmmo)
	{
		OnRep_CurrentAmmo();
	}
}

int32 AWeaponActor::GetCurrentAmmo() const
{
	return _CurrentAmmo;
}

int32 AWeaponActor::GetMaxAmmo() const
{
	return _WeaponAbilityData._BulletCount;
}

void AWeaponActor::OnRep_CurrentAmmo()
{
	_OnAmmoChanged.Broadcast(GetCurrentAmmo(), GetMaxAmmo());
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

	const FItemData* ItemData = TableSubsystem->FindTableRow<FItemData>(TEXT("ItemTable"), WeaponData->_WeaponId);
	if (nullptr == ItemData)
	{
		return false;
	}

	// 테이블 조회가 모두 성공한 뒤 무기 액터 내부에 복사
	_WeaponData = *WeaponData;
	// 반동 데이터의 Key 와 실제 조회에 사용한 Row Name을 일치시킨다
	_WeaponData._WeaponId = WeaponID;
	_WeaponData._Icon = ItemData->_Icon;
	_WeaponAbilityData = *WeaponAbilityData;

	if (HasAuthority())
	{
		_CurrentFireMode = _WeaponAbilityData.FireMode;
		_CurrentAmmo = _WeaponAbilityData._BulletCount;
		OnRep_CurrentAmmo();
	}

	_WeaponMesh->SetStaticMesh(_WeaponData._StaticMesh);

	// 무기 참조보다 데이터가 늦게 복제되어도 아이콘과 탄창 용량을 갱신한다.
	_OnWeaponDataChanged.Broadcast();

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

