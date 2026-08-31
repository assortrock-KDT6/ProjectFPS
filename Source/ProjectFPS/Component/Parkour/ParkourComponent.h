// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParkourComponent.generated.h"

class UAnimMontage;
class AController;
class UPrimitiveComponent;
class UHurdleCheckComponent;
class UMotionWarpingComponent;

struct FC_TraversalData;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UParkourComponent();

private:
	UPROPERTY()
	TObjectPtr<UHurdleCheckComponent>	HurdleCheckComponent;

	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
	
	TEnumAsByte<EMovementMode>			PreviousMovementMode = MOVE_Walking;

	TWeakObjectPtr<AController>			VaultController;

	// 현재 Vault로 통과중인 장애물 컴포넌트
	//UPROPERTY(Replicated)
	//TWeakObjectPtr<UPrimitiveComponent> VaultBlockComponent;

	/**
	 * 네트워크 복제 대상 X, 각 클라이언트가 World에서 얻은 컴포넌트. 
	 */
	TWeakObjectPtr<UPrimitiveComponent> LocalVaultBlockComponent;

	/**
	 * 서버만 증가시킨다. 
	 */
	uint16 NextTraversalActionId = 1;

	uint16 FinishedTraversalActionId = 0;

	/**
	 * 동일한 액션이 OnRep와 MovementMode 변경에서 두 번 재생되는 것을 막는다. 
	 */
	uint16 PresentedTraversalActionId = 0;

	/**
	 * 종료할 때 하드코딩된 이름 대신 실제 사용한 Warp Target을 제거한다.
	 */
	FName ActiveVaultWarpTargetName = NAME_None;

	FVector PresentedTraversalTarget = FVector::ZeroVector;

	// 중단되면 안전하게 시작 위치로 돌아가기 위한 값
	FTransform							VaultStartTransform;


	uint8								PreviousCustomMovementMode = 0;

protected:
	// Vault
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float MinVaultHeight = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float MaxVaultHeight = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float MaxVaultDepth = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", meta = (ClampMin = "0.0"))
	float VaultMontageStopBlendTime = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Network", meta = (ClampMin = "0.0"))
	float TraversalConfirmationTolerance = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Vault", Replicated)
	TObjectPtr<UAnimMontage> VaultMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Check")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|DEBUG")
	bool bDrawDebug = true;

	UPROPERTY(VisibleAnywhere, Category = "Parkour|Valut")
	bool bVaultActive = false;

	UPROPERTY(VisibleAnywhere, Category = "Parkour|Collision")
	bool bVaultBlockIgnore = false;
protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool TryParkour();
	
	bool IsVaultActive() const;

	void PlayVault(const FHitResult& FrontHit, const FHitResult& LandingHit);

	void OnVaultEnded(UAnimMontage* Montage, bool bInterrupted);

public:
	void EnterVaultPresentation(const FC_TraversalData& Data);
	void ExitValutPresentation();
public:
	UFUNCTION(Server, Reliable)
	void Server_TryParkour();
	void Server_TryParkour_Implementation();
private:
	float PlayValutMontage();

	void  ApplyLocalVaultCollisionIgnore(const FC_TraversalData& Data);
	void  ClearLocalVaultCollisionIgnore();
	float GetTraversalPresentationPosition(const FC_TraversalData& Data) const;
};
