// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Parkour/TraversalActionComponent.h"
#include "MantleComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UMantleComponent : public UTraversalActionComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMantleComponent();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Mantle|Trace")
	FMantleTraceSettings _TraceSettings;

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Mantle")
	float _HighMantleThreshold = 150.f;

public:
	virtual EProjectCustomMovementMode GetMode() const override;
	virtual int32	GetPriority() const override;
	virtual bool	BuildCandidate(const FTraversalBaseQuery& BaseQuery, FTraversalCandidate& OutCandidate) const;


//public:
//	bool TryMantle();
//
//	bool IsMantleActive() const;
//	
//private:
//	UPROPERTY()
//	TObjectPtr<UHurdleCheckComponent> HurdleCheckComponent;
//
//	UPROPERTY()
//	TObjectPtr<UAnimMontage> ActiveMantleMontage; // 현재 실행중인 몽타주를 저장할 변수
//
//	void PlayMantle(const FHitResult& FrontHit, const FHitResult& TopFloorHit);
//
//	void OnMantleEnded(UAnimMontage* Montage, bool bInterrupted);
//
//	bool bMantleActive = false;
//
//	TEnumAsByte<EMovementMode> PreviousMovementMode = MOVE_Walking;
//
//	uint8 PreviousCustomMovementMode = 0;
//
//	TWeakObjectPtr<AController> MantleController;
//
//	// Mantle 중 통과해야하는 장애물 컴포넌트
//	TWeakObjectPtr<UPrimitiveComponent> MantleBlockComponent;
//
//	// 중단되면 안전하게 시작 위치로 돌아가기 위한 값
//	FTransform MantleStartTransform;
//
//	bool bMantleBlockIgnore = false;
//
//protected:
//	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Mantle|Trace")
//	FMantleTraceSettings TraceSettings;
//
//protected:
//	// Called when the game starts
//	virtual void BeginPlay() override;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Mantle", meta = (ClampMin = "0.0"))
//	float MinMantleHeight = 80.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Mantle", meta = (ClampMin = "0.0"))
//	float Mantle2MHeight = 150.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Mantle", meta = (ClampMin = "0.0"))
//	float MaxMantleHeight = 250.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Mantle", meta = (ClampMin = "0.0"))
//	TObjectPtr<UAnimMontage> MantleMontage1M;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Mantle", meta = (ClampMin = "0.0"))
//	TObjectPtr<UAnimMontage> MantleMontage2M;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Mantle")
//	FName MantleWarpTargetName = TEXT("MantleTop");
};
