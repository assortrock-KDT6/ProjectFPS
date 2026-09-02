// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/GameDatas.h"
#include "Components/ActorComponent.h"
#include "HurdleCheckComponent.generated.h"

/*
* HurdleCheck Component 는 Parkour, Hanging, Mantle 모션을 취하기 위해서 사전 검사를 담당한다.
* PrimaryComponentTick.bCanEverTick = false 인 이유는 매 프로엠 스스로 검사하는게 아니라, 다른 액션 컴포넌트가 요청했을때만 
* Trace를 실행하기 떄문
*/

/** [변경사항]
 *	액션 상태나 네트워크를 소유하지 않는다. Trace 계산만 담당하며 복제하지 않는다.
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UHurdleCheckComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHurdleCheckComponent();

public:
	/**
	 * Vault 와 Mantle이 공유하는 Front.Top 검사는 한 번만 실행한다. 
	 */
	bool BuildBaseQuery(FTraversalBaseQuery& OutResult) const;

	/**
	 * Vault 장애물의 뒷면과 깊이를 검사한다.
	 */
	bool CheckVaultBackBlock(const FTraversalBaseQuery& BaseQuery, const FVaultTraceSettings& Settings, FHitResult& OutBackHit, float& OutDepth) const;
	//bool CheckVaultBackBlock(const FHitResult& FrontHit, FHitResult& TopHit, FHitResult& OutBackHit, float& OutDepth) const;
	
	/**
	 * Vault 통과 후 착지할 바닥을 검사한다.
	 */
	bool CheckVaultLandingFloor(const FTraversalBaseQuery& BaseQuery, const FHitResult& BackHit, const FVaultTraceSettings& Settings, FHitResult& OutLandingHit) const;
	//bool CheckVaultLandingFloor(const FHitResult& FrontHit, const FHitResult& TopHit, const FHitResult& BackHit, FHitResult& OutHit) const;

	/**
	 * Mantle 통과 후 올라설 윗면을 검사한다. 
	 */
	bool CheckMantleTopFloor(const FTraversalBaseQuery& BaseQuery, const FMantleTraceSettings& Settings, FHitResult& OutTopFloorHit) const;
	//bool CheckMantleTopFloor(const FHitResult& FrontHit, const FHitResult& TopHit, FHitResult& OutHit) const;
	
	/**
	 * 캐릭터 캡슐이 목표 위치에 들어갈 수 있는지 검사한다. 
	 */
	bool CheckLandingSpace(const FHitResult& LandingHit) const;

	/**
	 * 복제된 충돌점과 법선으로 로컬 장애물 컴포넌트를 찾는다. 
	 */
	bool ResolveLocalObstacle(const FVector& ObstaclePoint, const FVector& ObstacleNormal, FHitResult& OutHit) const;
	
	float GetLandingFloorClearance() const;

private:
	bool TraceFrontBlock(FHitResult& OutHit) const;

	bool TraceTopBlock(const FHitResult& FrontHit, FHitResult& OutTopHit, float& OutHeight) const;			// 검출한 윗면 정보를 돌려줌 -> 반환값 : 유효한 윗면을 찾았는지 알려줌

	bool IsUsableFloor(const FHitResult& FloorHit) const;

	bool IsSameObstacle(const FHitResult& FirstHit, const FHitResult& SecondHit) const;

	/**
	 * 벽타기 + 커스텀 중력을 달라지는 경우 사용.
	 */
	FQuat MakeCapsuleRotation(const FVector& UpVector);
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Hurdle|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Front Check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Hurdle|Front", meta = (ClampMin = "0.0"))
	float FrontCheckHalfHeight = 50.0f;

	// Top Check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Hurdle|Top", meta = (ClampMin = "0.0"))
	float TopCheckInset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace|Top", meta = (ClampMin = "0.0"))
	float MinTopHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace|Top", meta = (ClampMin = "0.0"))
	float MaxTopHeight = 250.0f;

	/**
	 * 공통 센서가 검사할 전체 높이 범위다.
	 * Vault와 Mantle이 실제 허용 범위는 각 액션 컴포넌트가 판단한다.
	 */

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Trace|Top", meta = (ClampMin = "0.0"))
	float MinTopTraceHeight = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Trace|Top", meta = (ClampMin = "0.0"))
	float MaxTopTraceHeight = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Trace|Landing", meta = (ClampMin = "0.0"))
	float LandingFloorClearance = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Trace|Collision", meta = (ClampMin = "0.0"))
	float ObstacleResolveTraceHalfDistance = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Debug")
	bool bDrawDebug = true;

private:
	inline static const float DebugLifeTime			= 2.f;
	inline static const float DebugLineThickness	= 2.f;
	inline static const float DebugPointRadius		= 8.f;
	inline static const int32 DebugPointSegments	= 12;

	//// Back Check
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Back", meta = (ClampMin = "0.0"))
	//float BackCheckDistance = 300.0f;			// 뒷면을 검색할 최대 거리

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Back", meta = (ClampMin = "0.0"))
	//float BackCheckHeight = 10.0f;				// 윗면에 걸리지 않도록 검사 높이를 아래로 내리는 값

	//// Floor Check
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	//float LandingCheckFront = 60.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	//float LandingCheckUp = 50.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	//float LandingCheckDown = 300.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Floor", meta = (ClampMin = "0.0"))
	//float LandingCapsule = 2.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|TopFloor", meta = (ClampMin = "0.0"))
	//float TopFloorCheckDistance = 50.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hurdle|Collision", meta = (ClampMin = "1.0"))
	//float ObstacleResolveTraceHalfDistance = 20.f;

};
