// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/CharacterBase.h"
#include "CharacterPlayer.generated.h"

/*
*	[ 플레이어 준비물 ] 
*	애니메이션, 스켈레탈메시	
* 
*	1) 플레이어는 입력을 받아서 움직인다 [ InputContext ]
*	2) 플레이어와 플레이어는 서로 공격할 수 있다. [ Team 설정 ] -> GameMode <-> PlayerState		
* 
*/
class UDefaultInput;
class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;
struct FInputActionValue;

UCLASS()
class PROJECTFPS_API ACharacterPlayer : public ACharacterBase
{
	GENERATED_BODY()
public:
	ACharacterPlayer(const FObjectInitializer& ObjectInitializer);
public:
	// 카메라 회전 감도 조절 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float _LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float _ZoomSensitivity = 30.f;

	// 줌 견착
	// 입력은 조준을 시작/해제한다 는 의도만 전달 -> 실제 화면 전환은 Tick에서 부드럽게 처리한다.
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void SetAiming(bool bAniming);


	// 카메라 중앙이 가리키는 월드 위치를 구하기
	FVector GetAnimPoint(float WeaponRange) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USpringArmComponent> _SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraComponent> _CameraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDefaultInput> _DefaultInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "First Person")
	TObjectPtr<USkeletalMeshComponent> _FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour")
	TObjectPtr<class UHurdleCheckComponent> _HurdleCheckComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour")
	TObjectPtr<class UVaultComponent> _VaultComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour")
	TObjectPtr<class UMantleComponent> _MantleComponent;

	UPROPERTY(VisibleAnywhere, Category = "Interact")
	TObjectPtr<class UInteractionComponent> _InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FPS | AbilitySystem | Attribute")
	TObjectPtr<class UFPSAttributeSet> _HealthAttribute;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void OnRep_PlayerState() override;
public:
	virtual void Jump() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* Newcontroller) override;

protected:
	UFUNCTION()
	void MoveAction(const FInputActionValue& Value);

	UFUNCTION()
	void MoveLookAction(const FInputActionValue& Value);

	UFUNCTION()
	void CharacterMouseZoomAction(const FInputActionValue& Value);

	UFUNCTION()
	virtual void ParkourAction(const struct FInputActionValue& Value);

	UFUNCTION()
	void ToggleInventoryAction(const FInputActionValue& value);
	
	UFUNCTION()
	void ToggleMapAction(const FInputActionValue& value);

	UFUNCTION()
	void InteractAction(const FInputActionValue& value);

private:
	void SetupPlayerMesh();
};
