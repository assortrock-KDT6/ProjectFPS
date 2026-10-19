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
class UNiagaraSystem;
class USoundBase;

// 모든 투척물의 공통 부모 (장착·투척·쿠킹·신관·폭발 연출·정리). 터질 때 하는 일은 자식 클래스가 Detonate로 정한다.
UCLASS(Abstract)
class PROJECTFPS_API AGrenadeActor : public AActor
{
	GENERATED_BODY()
	
protected:
	// 바닥·벽과 부딪히는 충돌. 던진 뒤 물리 시뮬레이션의 기준이 된다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<USphereComponent> _GrenadeCollision;	
	
	// 보이는 수류탄 메시. 충돌은 _GrenadeCollision이 담당한다
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

	// 신관 시간. 손을 떠난 순간부터 터질 때까지 (쿠킹하면 그만큼 줄어든다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Throw", meta = (ClampMin = "0.1", Units = "s"))
	float _FuseTime = 5.f;

	// 쿠킹 가능 여부. 닿으면 터지는 투척물(화염병)은 false
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade | Throw")
	bool _bCanCook = true;

	// 폭발 범위·데미지처럼 종류마다 다른 값은 각 자식 클래스가 가진다. (폭탄 = ABombGrenadeActor)

	// BP 클래스 기본값에서 사용할 폭발 이펙트를 지정한다. -> 나이아가라로 변경.
	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Explosion")
	TObjectPtr<UNiagaraSystem> _ExplosionEffect = nullptr;

	// BP 클래스 기본값에서 사용할 폭발음을 지정한다. 거리 감쇠와 볼륨은 사운드 에셋 설정을 따른다.
	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Explosion")
	TObjectPtr<USoundBase> _ExplosionSound = nullptr;

	// BP 클래스 기본값에서 쿠킹 시작 시 재생할 소리를 지정한다
	UPROPERTY(EditDefaultsOnly, Category = "Grenade | Sound")
	TObjectPtr<USoundBase> _CookSound = nullptr;

public:	
	AGrenadeActor();
	
	// 캐릭터의 1인칭 표시에도 같은 Mesh를 사용하기
	UStaticMeshComponent* GetGrenadeMesh() const;

	FName GetTID() const;

	float GetFuseTime() const;

	bool CanCook() const;
	
	// 서버에서 손으로부터 분리하고 초기 속도를 적용
	bool Throw(const FVector& Direction);
	
	// 쿠킹 -> 무기교체 일때 쿠킹된 수류탄이 인벤토리로 가지 않고 떨어지는 기능
	bool Drop();
	
	// 서버가 쿠킹 시작 위치를 전달하면 각 플레이어 쪽에서 소리를 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayCookSound(const FVector& Location);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;

	virtual void LifeSpanExpired() override;

	// 터졌을 때 하는 일. 서버에서만 호출된다. 종류별 자식 클래스가 바꾼다.
	virtual void Detonate(const FVector& ExplosionLocation);

	// 자식 클래스(화염병)가 던져진 뒤인지 확인할 때 사용
	bool IsThrown() const;
	
public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Thrown)
	bool _Thrown = false;
	
	UFUNCTION()
	void OnRep_Thrown();
	
	// 서버가 폭발 위치를 전달하면 각 화면에서 이펙트를 재생하기 위한 함수
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayExplosion(const FVector& Location);
};
