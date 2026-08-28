// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "WeaponTypes.generated.h"

/**
 *  변동 변수가 될 수 있는 값은 Unreal Engine의 GAS로 옮기고
 *  변동되지 않는 변수와 정의, 상태는 이곳에서 Table로 관리한다.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UWeaponTypes : public UObject
{
	GENERATED_BODY()
};


// FPS 무기 분류
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None		UMETA(DisplayName = "None"),
	Pistol		UMETA(DisplayName = "Pistol"),
	Rifle       UMETA(DisplayName = "Rifle"),
	Shotgun     UMETA(DisplayName = "Shotgun"),
	Sniper		UMETA(DisplayName = "Sniper")
};

// 사용할 무기의 따른 총알(Pojectile) 타입
UENUM(BlueprintType)
enum class EWeaponBulletType : uint8
{
	None			  UMETA(DisplayName = "None"),
	Bullet_PistolType UMETA(DisplayName = "Bullet_PistolType"),
	Bullet_RifleType  UMETA(DisplayName = "Bullet_RifleType"),
	Bullet_SniperType UMETA(DisplayName = "Bullet_SniperType")
};

// 입력에 따른 단발, 연발 구분
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	None	       UMETA(DisplayName = "None"),
	SemiAutomatic  UMETA(DisplayName = "SemiAutomatic"), // 단발
	Automatic      UMETA(DisplayName = "Automatic")		 // 연발
};

// 탄환 판정을 히트싱크 방식으로 할지, Projectile 방식으로 할지
UENUM(BlueprintType)
enum class EWeaponFireType : uint8
{
	HitScan	   UMETA(DisplayName = "HitScan"),
	Projectile UMETA(DisplayName = "Projectile")
};

// 무기상태
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	UnEquipped UMETA(DisplayName = "UnEquipped"),
	Idle       UMETA(DisplayName = "Idle"),
	Fire       UMETA(DisplayName = "Fire"),
	Reloading  UMETA(DisplayName = "Reloading")
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponInfomation | ID"))
	FName _WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbilInfomation | TID"))
	FName _WeaponAbilId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponInfomation | Type"))
	EWeaponType _WeaponType = EWeaponType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponInfomation | Icon"))
	TObjectPtr<UTexture2D> _Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisPlayName = "WeaponInfomation | StaticMesh"))
	TObjectPtr<UStaticMesh> _StaticMesh = nullptr;
};

// todo : 나중에 GAS 로 변동값 옮기기
USTRUCT(BlueprintType)
struct FWeaponAbilityDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | FireMode"))
	EWeaponFireMode FireMode = EWeaponFireMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | FireType"))
	EWeaponFireType FireType = EWeaponFireType::Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | BulletType"))
	EWeaponBulletType BulletType = EWeaponBulletType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | ProjectileInterval"))
	float _ProjectileInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | Damage"))
	float _Damage = 13.0f;   // 한발당 13의 데미지로 체력 100인 플레이어가 0.1초간격으로 8발을 맞으면 사망

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | Range"))
	float _Range = 10000.0f; // 사거리 10000cm = 100m

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | ReloadTime"))
	float _ReloadTime = 2.0f; // 2초

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | BulletCount",
		                                                                     ClampMin    = "0" , ClampMax = "30", 
																			 UIMin       = "0" , UIMax    = "30"))
	uint8 _BulletCount = 30;
};
