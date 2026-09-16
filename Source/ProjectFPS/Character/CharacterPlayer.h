// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/CharacterBase.h"
#include "TimerManager.h"				// 총알 연사를 구현하기 위해서 넣었습니다. - 건영 
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
class AWeaponActor;
class AWeaponPickUp;
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
	FVector GetAimPoint(float WeaponRange) const;

	// 무기를 생성하고 초기화한 뒤, FirstPersonMesh에 장착
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeapon(FName WeaponID);
	
	// 무기 장착 성공 후에 Blueprint에 애니메이션 상태 변경을 알리기
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnWeaponEquiped();

	// WeaponPickUp의 상호작용 범위에 들어온 무기를 등록한다.
	void SetNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp);
	
	// 등록된 WeaponPickUp의 범위에서 벗어나면 해제된다.
	void ClearNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USpringArmComponent> _SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraComponent> _CameraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDefaultInput> _DefaultInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "First Person")
	TObjectPtr<USkeletalMeshComponent> _FirstPersonMesh;
	
	// 장착되는 모든 총기의 공통 Actor 클래스다. 시작 무기를 의미하지않는다.
	// 실제 무기 Actor를 생성할 공통 Blueprint 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AWeaponActor> _WeaponActorClass;
	// 현재 장착된 무기
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AWeaponActor> _CurrentWeapon;
	
	// 현재 상호작용 범위 안에 있는 월드 무기
	UPROPERTY()
	TObjectPtr<AWeaponPickUp> _NearbyWeaponPickUp;
	
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

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual void Jump() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* Newcontroller) override;

public:
	USkeletalMeshComponent* Get_FirstPersonMesh() const;
	USkeletalMeshComponent* Get_ThirtPersonMesh() const;

protected:
	UFUNCTION()
	void MoveAction(const FInputActionValue& Value);

	UFUNCTION()
	void MoveLookAction(const FInputActionValue& Value);

	UFUNCTION()
	void CharacterMouseZoomAction(const FInputActionValue& Value);

	UFUNCTION()
	virtual void ParkourAction(const FInputActionValue& Value);

	UFUNCTION()
	void ToggleInventoryAction(const FInputActionValue& value);
	
	UFUNCTION()
	void ToggleMapAction(const FInputActionValue& value);

	UFUNCTION()
	void InteractAction(const FInputActionValue& value);

	UFUNCTION()
	void HandleOutOfHealth();

	UFUNCTION()
	void FireAction(const FInputActionValue& value);

	UFUNCTION()
	void StopFireAction(const FInputActionValue& value);

	UFUNCTION()
	void FireToggleAction(const FInputActionValue& value);

	UFUNCTION(Server, Reliable)
	void ServerStartFire();
	void ServerStartFire_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerStopFire();
	void ServerStopFire_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerToggleFireMode();
	void ServerToggleFireMode_Implementation();
	
private:
	// 입력함수로 서버에서 시작, 정지만 요청하고 타이머로 발사관리하면서 FireOnce()는 실제로 한발만 발사합니다. 책임을 겹치지 않게 나눈거에요 
	// 서버에서 연사 간격을 관리하는 타이머
	FTimerHandle _FireTimerHandle;
	
	// 한 발의 조준점 계산과 발사를 실행
	void FireOnce();
	
	void SetupPlayerMesh();
};
