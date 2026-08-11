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
	bool CheckFrontBlock(FHitResult& OutHit) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float FrontCheckHalfHeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|DEBUG")
	bool bDrawDebug = true;
};
