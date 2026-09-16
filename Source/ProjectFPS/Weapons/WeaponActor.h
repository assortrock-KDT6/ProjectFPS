// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponInterface.h"
#include "WeaponTypes.h"
#include "WeaponActor.generated.h"

UCLASS()
class PROJECTFPS_API AWeaponActor : public AActor, public IWeaponInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponActor();

	// WeaponID 에 해당하는 기본 정보와 능력치를 한번 조회하고 캐싱하기
	// Implementation 은 IWeaponInterface 구현을 위한 코드 [ WeaponInterface에 Initialize 코드가 있어요 ] 
	virtual bool InitializeWeapon_Implementation(FName _WeaponID) override;
	
	// Muzzle Socket의 월드 위치를 반환
	FVector GetMuzzleLocation() const;
	
	// 현재 무기의 데이터 테이블 사거리를 반환
	float GetWeaponRange() const;
	
	// 현재 무기의 발사 모드를 반환 
	EWeaponFireMode GetFireMode() const;
	
	// 데이터 테이블에 설정된 발사 간격을 반환 ( = _ProjectileInterval)
	float GetProjectileInterval() const;
	
	// 현재 무기의 발사 모드를 단발, 연발 사이에서 전환 --> Todo : 단발과 연발이 없는 무기를 구분해야함 -> 이건 무기에 FireType 으로 받아와서 잡아주거나 Ability에 넣어서 bool 값으로 리턴하면 될듯합니다.
	void ToggleFireMode();
	
	// 총구에서 AimPoint 방향으로 Projectile Fire
	bool Fire(const FVector& AimPoint);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> _WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon | Data")
	FWeaponData _WeaponData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon | Data")
	FWeaponAbilityDataTable _WeaponAbilityData;
	
	// Blueprint에서 실제 발사할 EffectArea 기반 Projectile 을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon | Projectile")
	TSubclassOf<AActor> _ProjectileClass;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};