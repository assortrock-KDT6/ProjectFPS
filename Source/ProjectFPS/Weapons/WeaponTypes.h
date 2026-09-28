// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataTable.h"
#include "WeaponTypes.generated.h"

/**
 *  변동 변수가 될 수 있는 값은 Unreal Engine의 GAS로 옮기고
 *  변동되지 않는 변수와 정의, 상태는 이곳에서 Table로 관리한다.
 */

class USkeletalMesh;
class UAnimInstance;

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
	Sniper		UMETA(DisplayName = "Sniper"),
	Grenade     UMETA(DisplayName = "Grenade"),
	Smoke       UMETA(DisplayName = "Smoke")
};

// 사용할 무기의 따른 총알(Projectile) 타입
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

// 탄퍼짐 설정. 각도 값의 단위는 도(Degree)
USTRUCT(BlueprintType)
struct FWeaponSpreadInfo
{
	GENERATED_BODY()
	
	// 서서 일반 사격할 때의 기본 탄퍼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0.0"))
	float Hip = 2.f;

	// 조준 중 탄퍼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0.0"))
	float ADS = 0.f;

	// 앉아 있을 때의 기본 탄퍼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0.0"))
	float Crouch = 1.f;

	// 공중에 있을 때의 탄퍼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0.0"))
	float Fall = 4.f;

	// 서서 이동할 때 추가되는 탄퍼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0.0"))
	float Additive_Walk = 1.f;

	// 발사 누적 횟수당 추가되는 탄퍼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0.0"))
	float Additive_Recoil = 0.2f;

	// 탄퍼짐 계산에 반영할 최대 발사 누적 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0"))
	int32 RecoilOffsetMax = 10;
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
	
	// 내 화면에표시할 1인칭 총기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | View")
	TObjectPtr<USkeletalMesh> _ViewMesh = nullptr;
	
	// 해당 총기 SkeletalMesh의 호환되는 애니메이션 블루프린트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon| View")
	TSubclassOf<UAnimInstance> _ViewAnimationInstance;
	
	// 줌 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Aim")
	bool _CanAim = false;
	
	// 기본 카메라 FOV 의 곱할 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Aim", meta = (ClampMin = "0.01", EditCondition = "_CanAim"))
	float _AimFOVMultiplier = 1.f;
	
	// 곱셈 계산 후 추가할 FOV 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Aim", meta = (EditCondition = "_CanAim"))
	float _AimFOVAdditive = 0.f;
	
	// 조준 중 팔과 총기 머테리얼에 전달할 수평 FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Aim", meta=(ClampMin="1.0", ClampMax="179.0", EditCondition="_CanAim"))
	float _AimViewFOV = 80.f;
	
	// 기본 마우스 감도에 곱할 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Aim", meta = (ClampMin = "0.01", EditCondition = "_CanAim"))
	float _AimSensitivityMultiplier = 1.f;
	
	// 무기의 상태별 탄퍼짐 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Spead")
	FWeaponSpreadInfo _SpreadInformation;

	bool IsValid() const
	{
		return	_WeaponId != NAME_None && _WeaponAbilId != NAME_None && _WeaponType != EWeaponType::None
				&& _StaticMesh != nullptr && _ViewMesh != nullptr /*&& _ViewAnimationInstance != nullptr && _Icon != nullptr*/;
	}
};

// todo : 나중에 GAS 로 변동값 옮기기
USTRUCT(BlueprintType)
struct FWeaponAbilityDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "WeaponAbility | FireMode"))
	EWeaponFireMode FireMode = EWeaponFireMode::None;

	// Empty keeps only the default FireMode available for existing rows.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<EWeaponFireMode> SupportedFireModes;

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
