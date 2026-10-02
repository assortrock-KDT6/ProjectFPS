// Fill out your copyright notice in the Description page of Project Settings.

/*
 *	무기가 WeaponActor 인데 GrenadeActor를 만든 이유는 WeaponActor는 WeaponTable이랑 Weapon이 아니면 그냥 Return 시켜서 터져요
 *	처음 Grenade를 ItemTable에 넣어서 시작했으니 이렇게 따로 빼서 작업했습니다.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadeActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class PROJECTFPS_API AGrenadeActor : public AActor
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<USphereComponent> _GrenadeCollision;	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UStaticMeshComponent> _GrenadeMesh;
	
	// ItemTable에서 이 수류탄을 식별하는 행 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	FName _TID = NAME_None;
	
	// 투척 초기 속도
	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Throw", meta = (ClampMin = "1.0", Units = "cm/s"));
	float _ThrowSpeed = 1300.f;
	
public:	
	AGrenadeActor();
	
	// 캐릭터의 1인칭 표시에도 같은 Mesh를 사용하기
	UStaticMeshComponent* GetGrenadeMesh() const;

	FName GetTID() const;
	
	// 서버에서 손으로부터 분리하고 초기 속도를 적용
	bool Throw(const FVector& Direction);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Thrown)
	bool _Thrown = false;
	
	UFUNCTION()
	void OnRep_Thrown();
};
