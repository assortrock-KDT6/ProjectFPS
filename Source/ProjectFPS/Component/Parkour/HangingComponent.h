// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HangingComponent.generated.h"

class UAnimMontage;
class AController;
class UPrimitiveComponent;
class UHurdleCheckComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UHangingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHangingComponent();

public:
	bool TryHanging();

private:
	UPROPERTY()
	TObjectPtr<UHurdleCheckComponent> HurdleCheckComponent;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveHangingMontage; // 현재 실행중인 몽타주를 저장할 변수

	void PlayHanging(const FHitResult& FrontHit, const FHitResult& TopFloorHit);

	void OnHangingEnded(UAnimMontage* Montage, bool bInterrupted);

	bool bHangingActive = false;

	TEnumAsByte<EMovementMode> PreviousMovementMode = MOVE_Walking;

	uint8 PreviousCustomMovementMode = 0;

	TWeakObjectPtr<AController> HangingController;

	// Hanging 중 통과해야하는 장애물 컴포넌트
	TWeakObjectPtr<UPrimitiveComponent> HangingBlockComponent;

	// 중단되면 안전하게 시작 위치로 돌아가기 위한 값
	FTransform HangingStartTransform;

	bool bHangingBlockIgnore = false;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
