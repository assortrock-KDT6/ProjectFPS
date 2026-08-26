// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParkourComponent.generated.h"

class UAnimMontage;
class AController;
class UPrimitiveComponent;
class UHurdleCheckComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UParkourComponent();

public:
	bool TryParkour();
	
	bool IsVaultActive() const;
	
private:
	UPROPERTY()
	TObjectPtr<UHurdleCheckComponent> HurdleCheckComponent;

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
	virtual void BeginPlay() override;

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
