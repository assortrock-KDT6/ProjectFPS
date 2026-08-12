// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParkourComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UParkourComponent();

public:
	void TryParkour();

private:
	bool CheckFrontBlock(FHitResult& OutHit) const;															// 장애물의 앞면 검사 결과를 입력
	
	bool CheckTopBlock(const FHitResult& FrontHit, FHitResult& OutHit, float& OutHeight) const;			// 검출한 윗면 정보를 돌려줌 -> 반환값 : 유효한 윗면을 찾았는지 알려줌

	bool CheckBackBlock(const FHitResult& FrontHit, FHitResult& TopHit, FHitResult& OutBackHit, float& OutDepth) const;

protected:
	// Front
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckHalfHeight = 50.0f;


	// Top
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float TopCheckInset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float MinParkourHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float MaxParkourHeight = 250.0f;

	// Back
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float MaxVaultDepth = 200.0f;			// Vault 로 넘을 수 있는 장애물의 최대 두께 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float BackCheckHeightOffset = 10.0f;	// 윗면에 걸리지 않도록 검사 높이를 아래로 내리는 값


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|DEBUG")
	bool bDrawDebug = true;
};
