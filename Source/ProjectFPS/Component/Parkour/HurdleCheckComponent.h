// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HurdleCheckComponent.generated.h"

/*
* HurdleCheck Component 는 Parkour, Hanging, Mantle 모션을 취하기 위해서 사전 검사를 담당한다.
* PrimaryComponentTick.bCanEverTick = false 인 이유는 매 프로엠 스스로 검사하는게 아니라, 다른 액션 컴포넌트가 요청했을때만 
* Trace를 실행하기 떄문
*/

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UHurdleCheckComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHurdleCheckComponent();

public:
	// Parkour : Public
	bool CheckFrontBlock(FHitResult& OutHit) const;

	bool CheckTopBlock(const FHitResult& FrontHit, FHitResult& OutHit, float& OutHeight) const;			// 검출한 윗면 정보를 돌려줌 -> 반환값 : 유효한 윗면을 찾았는지 알려줌

	bool CheckLandingSpace(const FHitResult& LandingHit) const;

	// Parkour : Vault
	bool CheckBackBlock(const FHitResult& FrontHit, FHitResult& TopHit, FHitResult& OutBackHit, float& OutDepth) const;

	bool CheckLandingFloor(const FHitResult& FrontHit, const FHitResult& TopHit, const FHitResult& BackHit, FHitResult& OutHit) const;

	// Parkour : Mantle
	bool CheckTopFloor(const FHitResult& FrontHit, const FHitResult& TopHit, FHitResult& OutHit) const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Debug")
	bool bDrawDebug = true;

	// Front Check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckHalfHeight = 50.0f;

	// Top Check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Top", meta = (ClampMin = "0.0"))
	float TopCheckInset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Top", meta = (ClampMin = "0.0"))
	float MinTopHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Top", meta = (ClampMin = "0.0"))
	float MaxTopHeight = 250.0f;

	// Back Check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Back", meta = (ClampMin = "0.0"))
	float BackCheckDistance = 300.0f;			// 뒷면을 검색할 최대 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Back", meta = (ClampMin = "0.0"))
	float BackCheckHeight = 10.0f;				// 윗면에 걸리지 않도록 검사 높이를 아래로 내리는 값

	// Floor Check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	float LandingCheckFront = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	float LandingCheckUp = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	float LandingCheckDown = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	float LandingCapsule = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|TopFloor", meta = (ClampMin = "0.0"))
	float TopFloorCheckDistance = 50.0f;

};
