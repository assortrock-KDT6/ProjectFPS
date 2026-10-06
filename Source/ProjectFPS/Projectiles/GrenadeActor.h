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
class UGameplayEffect;

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Throw", meta = (ClampMin = "1.0", Units = "cm/s"))
	float _ThrowSpeed = 1300.f;
	
	// 실제 투척 각도와 예측 라인이 같이 사용할 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Throw", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float _ThrowAngle = 20.f;
	
	// 폭발 피해를 검색할 반경, 단위는 cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Explosion", meta = (ClampMin = "1.0", Units = "cm"))
	float _ExplosionRadius = 500.f;
	
	// 기존 피해 GE에 전달할 수류탄의 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Explosion", meta = (ClampMin = "0.0"))
	float _Damage = 150.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Explosion")
	TSubclassOf<UGameplayEffect> _DamageEffect;
	
public:	
	AGrenadeActor();
	
	// 캐릭터의 1인칭 표시에도 같은 Mesh를 사용하기
	UStaticMeshComponent* GetGrenadeMesh() const;

	FName GetTID() const;
	
	// 서버에서 손으로부터 분리하고 초기 속도를 적용
	bool Throw(const FVector& Direction);
	
	// 쿠킹 -> 무기교체 일때 쿠킹된 수류탄이 인벤토리로 가지 않고 떨어지는 기능
	bool Drop();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;

	virtual void LifeSpanExpired() override;
	
public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Thrown)
	bool _Thrown = false;
	
	UFUNCTION()
	void OnRep_Thrown();
};
