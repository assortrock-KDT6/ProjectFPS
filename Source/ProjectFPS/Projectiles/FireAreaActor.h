// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireAreaActor.generated.h"

class APlayerStateBase;
class UGameplayEffect;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;

// 화염병이 깨진 자리에 깔리는 불 장판. 작게 깔렸다가 점점 넓어진다.
// 서버가 일정 간격으로 범위 안의 폰에게 데미지를 주고, 각 화면은 불 이펙트·소리를 재생한다.
UCLASS()
class PROJECTFPS_API AFireAreaActor : public AActor
{
	GENERATED_BODY()

protected:
	// 불 장판 최대 반경. 이 범위 안에 있는 폰이 데미지를 받는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "1.0", Units = "cm"))
	float _Radius = 250.f;

	// 처음 깔릴 때 반경. _SpreadTime 동안 _Radius까지 넓어진다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.0", Units = "cm"))
	float _StartRadius = 60.f;

	// 최대 반경까지 넓어지는 데 걸리는 시간. 0이면 처음부터 최대 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.0", Units = "s"))
	float _SpreadTime = 1.5f;

	// 불이 타는 시간. 끝나면 장판이 사라진다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.1", Units = "s"))
	float _Duration = 6.f;

	// 데미지를 주는 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.05", Units = "s"))
	float _TickInterval = 0.5f;

	// 한 번(_TickInterval마다)에 주는 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.0"))
	float _DamagePerTick = 8.f;

	// 범위 안 대상에게 적용할 피해 GE. _DamagePerTick 값을 SetByCaller로 넘긴다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	TSubclassOf<UGameplayEffect> _DamageEffect;

	// 장판이 있는 동안 재생할 불 이펙트 (반복 재생되는 나이아가라)
	UPROPERTY(EditDefaultsOnly, Category = "Fire | Effect")
	TObjectPtr<UNiagaraSystem> _FireEffect = nullptr;

	// 불 이펙트 크기. 에셋마다 기본 크기가 달라서 _Radius에 맞게 조절한다
	UPROPERTY(EditDefaultsOnly, Category = "Fire | Effect")
	FVector _FireEffectScale = FVector(1.f);

	// 현재 반경을 넘겨받을 나이아가라 User 파라미터 이름 (NS에 float User 파라미터를 만들고 Shape Location의 반경에 연결)
	UPROPERTY(EditDefaultsOnly, Category = "Fire | Effect")
	FName _EffectRadiusParameter = TEXT("FireRadius");

	// 장판이 있는 동안 반복 재생할 불타는 소리 (사운드 에셋에서 Looping 켜기)
	UPROPERTY(EditDefaultsOnly, Category = "Fire | Effect")
	TObjectPtr<USoundBase> _FireLoopSound = nullptr;

private:
	// 처치 기록에 사용할 화염병을 던진 사람
	TWeakObjectPtr<APlayerStateBase> _SourcePlayerState;

	// 서버에서 데미지를 반복해서 주는 타이머
	FTimerHandle _DamageTimerHandle;

	// 장판이 깔린 시간. 서버·클라 각자 자기 BeginPlay 기준으로 넓어진다
	float _SpawnTime = 0.f;

	// 넓어지는 동안 반경을 넘겨줄 불 이펙트
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> _FireEffectComponent = nullptr;

public:
	AFireAreaActor();

	// 생성 직후(FinishSpawning 전에) 서버가 넘겨준다.
	void SetSourcePlayerState(APlayerStateBase* PlayerState);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 넓어지는 동안만 불 이펙트에 반경을 넘겨준다. 다 넓어지면 Tick을 끈다.
	virtual void Tick(float DeltaSeconds) override;

private:
	// 깔린 뒤 지난 시간에 따른 지금 반경
	float GetCurrentRadius() const;

	// 지금 반경을 불 이펙트에 넘겨준다. (이펙트 크기 배율만큼 나눠서 실제 반경과 맞춘다)
	void UpdateEffectRadius();

	// 범위 안의 폰에게 한 번 데미지를 준다. (서버)
	void ApplyFireDamage();
};
