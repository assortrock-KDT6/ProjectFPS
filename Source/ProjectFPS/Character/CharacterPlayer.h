// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/CharacterBase.h"
#include "TimerManager.h"				// 총알 연사를 구현하기 위해서 넣었습니다. - 건영 
#include "Weapons/WeaponTypes.h"
#include "CharacterPlayer.generated.h"


/*
*	[ 플레이어 준비물 ] 
*	애니메이션, 스켈레탈메시	
* 
*	1) 플레이어는 입력을 받아서 움직인다 [ InputContext ]
*	2) 플레이어와 플레이어는 서로 공격할 수 있다. [ Team 설정 ] -> GameMode <-> PlayerState		
* 
*	----
*	무기 장착의 크로스헤어 변환
*	1. 무기를 장착하게되면 Delegate 를 통해서 크로스헤어 위젯에 무기 장착여부를 알림
*	2. 크로스헤어 위젯은 Delegate를 통해 무기 장착여부를 확인한 뒤 . 에서 + 로 크로스헤어를 변경
*/
class UDefaultInput;
class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class AWeaponActor;
class AWeaponPickUp;
class UFPSViewSkeletalMeshComponent;
class UControlShakeComponent;
class USkeletalMesh;
class UAnimInstance;
class AItemPickUp;
class APlayerStateBase;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponEquippedChangedSignature, bool, bEquipped);


UCLASS()
class PROJECTFPS_API ACharacterPlayer : public ACharacterBase
{
	GENERATED_BODY()
public:
	ACharacterPlayer(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float _LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float _ZoomSensitivity = 30.f;

	// 줌 견착
	// 입력은 조준을 시작/해제한다 는 의도만 전달 -> 실제 화면 전환은 Tick에서 부드럽게 처리한다.
	// UFUNCTION(BlueprintCallable, Category = "Aim")
	// void SetAiming(bool bAiming);
	
	// 무기 장착 상태가 변경되면 로컬 UI에 알리기
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FWeaponEquippedChangedSignature WeaponEquppedChanged;
	
	// UI가 생성될 때 현재 무기 장착 상태를 확인하기
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsWeaponEquipped() const;
	
	// 실제 사격과 크로스헤어가 함게 사용할 현재 탄퍼짐의 정도
	UFUNCTION(BlueprintPure, Category = "Weapon | Spread")
	float GetWeaponSpreadValud() const;
	
	// 무기를 생성하고 초기화한 뒤, FirstPersonMesh에 장착
	//UFUNCTION(BlueprintCallable, Category = "Weapon")
	//bool EquipWeapon(FName WeaponID);
	
	// 무기 장착 성공 후에 Blueprint에 애니메이션 상태 변경을 알리기
	//UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	//void OnWeaponEquiped();

	//// WeaponPickUp의 상호작용 범위에 들어온 무기를 등록한다.
	//void SetNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp);
	
	//// 등록된 WeaponPickUp의 범위에서 벗어나면 해제된다.
	//void ClearNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USpringArmComponent> _SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraComponent> _CameraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDefaultInput> _DefaultInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "First Person")
	TObjectPtr<UFPSViewSkeletalMeshComponent> _FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "First Person")
	TObjectPtr<UFPSViewSkeletalMeshComponent> _ViewWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recoil")
	TObjectPtr<UControlShakeComponent> _ControlShakeManager;

	// 장착되는 모든 총기의 공통 Actor 클래스다. 시작 무기를 의미하지않는다.
	// 실제 무기 Actor를 생성할 공통 Blueprint 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AWeaponActor> _WeaponActorClass;

	// 현재 장착된 무기
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
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

	// 준비만 되고 아직 확정 되지 않은 무기
	UPROPERTY()
	TObjectPtr<AWeaponActor> _PendingWeapon;

	// 버릴 때 캐릭터 기준으로 떨어질 범위
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float _DropForwardOffset = 100.f;

	// 빙의 해제 직후 도착하는 피해에도 원래 플레이어를 식별한다.
	TWeakObjectPtr<APlayerStateBase> _CombatPlayerState;

private:
	// 현재 플레이어가 조준을 유지하는지
	UPROPERTY(BlueprintReadOnly, Category = "Aim", meta = (AllowPrivateAccess = "true"))
	bool _bAiming = false;

	// 로컬 화면에서 사용할 현재 무기 설정
	UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	FWeaponData _ViewWeaponData;

	// 시작 시 카메라에 설정된 기본 FOV 와 현재 목표 FOV
	float _BaseCameraFOV = 90.f;
	float _TargetCameraFOV = 90.f;

	// 입력함수로 서버에서 시작, 정지만 요청하고 타이머로 발사관리하면서 FireOnce()는 실제로 한발만 발사합니다. 책임을 겹치지 않게 나눈거에요 
	// 서버에서 연사 간격을 관리하는 타이머
	FTimerHandle _FireTimerHandle;
	FTimerHandle _FiringTagTimerHandle;

public:
	// 줌 견착
	// 입력은 조준을 시작/해제한다 는 의도만 전달 -> 실제 화면 전환은 Tick에서 부드럽게 처리한다.
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void SetAiming(bool bAniming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bAiming);
	void ServerSetAiming_Implementation(bool bAiming);
	// 카메라 중앙이 가리키는 월드 위치를 구하기
	FVector GetAimPoint(float WeaponRange) const;

	// 실제 무기는 3인칭 오른손에 장착하고, 로컬 표시용 무기는 별도로 설정한다.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeapon(FName WeaponID);

	AWeaponActor* GetEquippedWeapon() const 
	{ 
		return _CurrentWeapon;
	}
	bool CanFireFromAbility() const;

	void NotifyAbilityWeaponFired(AWeaponActor* Weapon);

	void StopFiringPresentation();
	
	// 무기 장착 성공 후에 Blueprint에 애니메이션 상태 변경을 알리기
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnWeaponEquiped(EWeaponType WeaponType);

	// WeaponPickUp의 상호작용 범위에 들어온 무기를 등록한다.
	void SetNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp);
	
	// 등록된 WeaponPickUp의 범위에서 벗어나면 해제된다.
	void ClearNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp);

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

public:
	// 손에 든 무기 액터 비우기 
	void ClearEquippedWeapon();

	// 지정 슬롯의 무기를 손에 든다(서버)
	bool TryEquipSlot(int32 Index);

	// 지정 슬롯의 무기를 인벤에서 빼고 발미에 놓는다 (서버)
	bool TryDropWeaponAt(int32 Index);

public:
	void StopCombat();
	
	void StopAttacking();
	
	void InitializeAfterRespawn();
	
	APlayerStateBase* GetCombatPlayerState() const;

public:
	USkeletalMeshComponent* Get_FirstPersonMesh() const;
	USkeletalMeshComponent* Get_ThirtPersonMesh() const;

protected:
	UFUNCTION()
	void MoveAction(const FInputActionValue& Value);

	UFUNCTION()
	void MoveLookAction(const FInputActionValue& Value);

	UFUNCTION()
	void AimZoomAction(const FInputActionValue& Value);

	UFUNCTION()
	virtual void ParkourAction(const FInputActionValue& Value);

	UFUNCTION()
	void ToggleInventoryAction(const FInputActionValue& value);
	
	UFUNCTION()
	void ToggleMapAction(const FInputActionValue& value);

	UFUNCTION()
	void InteractAction(const FInputActionValue& value);

	UFUNCTION()
	void HandleOutOfHealth(APlayerStateBase* KillerPlayerState);

	UFUNCTION()
	void FireAction(const FInputActionValue& Value);

	UFUNCTION()
	void StopFireAction(const FInputActionValue& value);

	UFUNCTION()
	void FireToggleAction(const FInputActionValue& value);

	
	// 서버에서 결정한 1인칭 외형을 소유 플레이어에게 전달하기 + 기존에는 무기 외형뿐이였는데 조준설정도 같이 받기
	UFUNCTION(Client, Reliable)
	//void ClientSetViewWeapon(USkeletalMesh* ViewMesh, TSubclassOf<UAnimInstance> ViewAnimationClass);
	void ClientSetViewWeapon(const FWeaponData& WeaponData);
	
	// 서버에서 실제 발사가 성공했을때만 호출
	UFUNCTION(Client, Reliable)
	void ClientWeaponFired(FName WeaponID);


	UFUNCTION()
	void DropItemAction(const FInputActionValue& value);

	// x키로 장착무기 버리기
	UFUNCTION(Server, Reliable)
	void ServerDropEquippedWeapon();
	void ServerDropEquippedWeapon_Implementation();

public:
	
	// 인벤 UI우클릭 버리기 (클라 -> 서버)
	UFUNCTION(Server, Reliable)
	void ServerDropWeaponAt(int32 Index);
	void ServerDropWeaponAt_Implementation(int32 Index);

	UFUNCTION(server, Reliable)
	void ServerDropItemAt(int32 Index, int32 Count);
	void ServerDropItemAt_Implementation(int32 Index, int32 Count);

	// 지정 소모품 슬롯에서 Count만큼 빼서 발밑으로(서버)
	bool TryDropItemAt(int32 Index, int32 Count);

	UFUNCTION()
	void OnRep_CurrentWeapon();
	
private:
	void ClearFiringTag();
	
	// 한 발의 조준점 계산과 발사를 실행
	void FireOnce();
};
