// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParkourComponent.generated.h"

class UAnimMontage;
class AController;
class UPrimitiveComponent;

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
	bool CheckFrontBlock(FHitResult& OutHit) const;														// 장애물의 앞면 검사 결과를 입력

	bool CheckTopBlock(const FHitResult& FrontHit, FHitResult& OutHit, float& OutHeight) const;			// 검출한 윗면 정보를 돌려줌 -> 반환값 : 유효한 윗면을 찾았는지 알려줌

	bool CheckBackBlock(const FHitResult& FrontHit, FHitResult& TopHit, FHitResult& OutBackHit, float& OutDepth) const;

	bool CheckLandingFloor(const FHitResult& FrontHit, const FHitResult& TopHit, const FHitResult& BackHit, FHitResult& OutHit) const;

	bool CheckLandingSpace(const FHitResult& LandingHit) const;

	void PlayVault(const FHitResult& FrontHit, const FHitResult& LandingHit);

	void OnVaultEnded(UAnimMontage* Montage, bool bInterrupted);

	bool bVaultActive = false;

	TEnumAsByte<EMovementMode> PreviousMovementMode = MOVE_Walking;

	uint8 PreviousCustomMovementMode = 0;

	TWeakObjectPtr<AController> VaultController;

	// 현재 Vault로 통과중인 장애물 컴포넌트
	TWeakObjectPtr<UPrimitiveComponent> VaultBlockComponent;

	// 중단되면 안전하게 시작 위치로 돌아가기 위한 값
	FTransform VaultStartTransform;

	bool bVaultBlockIgnore = false;

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
	float BackCheckDistance = 300.0f;			// 뒷면을 검색할 최대 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float BackCheckHeight = 10.0f;				// 윗면에 걸리지 않도록 검사 높이를 아래로 내리는 값


	// Floor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float LandingCheckFront = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float LandingCheckUp = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float LandingCheckDown = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check", meta = (ClampMin = "0.0"))
	float LandingCapsule = 2.0f;


	// Vault
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float MinVaultHeight = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float MaxVaultHeight = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float MaxVaultDepth = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault")
	TObjectPtr<UAnimMontage> VaultMontage;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|DEBUG")
	bool bDrawDebug = true;
};
