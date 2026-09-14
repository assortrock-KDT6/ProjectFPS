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
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> _WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon | Data")
	FWeaponData _WeaponData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon | Data")
	FWeaponAbilityDataTable _WeaponAbilityData;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};