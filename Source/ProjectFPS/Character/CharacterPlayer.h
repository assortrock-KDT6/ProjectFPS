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
class UStaticMeshComponent;
class AWeaponActor;
class AGrenadeActor;
class AWeaponPickUp;
class UFPSViewSkeletalMeshComponent;
class UControlShakeComponent;
class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;
class AItemPickUp;
class APlayerStateBase;
struct FInputActionValue;

enum class EAnimMeshType : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponEquippedChangedSignature, bool, bEquipped);


UCLASS()
class PROJECTFPS_API ACharacterPlayer : public ACharacterBase
{
	GENERATED_BODY()
public:
	ACharacterPlayer(const FObjectInitializer& ObjectInitializer);

	// 무기 장착 상태가 변경되면 로컬 UI에 알리기
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FWeaponEquippedChangedSignature WeaponEquppedChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float _LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float _ZoomSensitivity = 30.f;

	// 줌 견착
	// 입력은 조준을 시작/해제한다 는 의도만 전달 -> 실제 화면 전환은 Tick에서 부드럽게 처리한다.
	// UFUNCTION(BlueprintCallable, Category = "Aim")
	// void SetAiming(bool bAiming);
	
	// UI가 생성될 때 현재 무기 장착 상태를 확인하기
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsWeaponEquipped() const;
	
	// 실제 사격과 크로스헤어가 함게 사용할 현재 탄퍼짐의 정도
	UFUNCTION(BlueprintPure, Category = "Weapon | Spread")
	float GetWeaponSpreadValue() const;
	
	// AnimBP 에서 사용할 순수 시선 입력을 조회하는 조회용 함수
	UFUNCTION(BlueprintPure, Category = "Input")
	FVector2D GetLookInput() const;
	// AnimBP 용 시선 입력 조회 인터페이스를 선언 -> 실제 입력은 EnhancedInput 에서 읽으며 캐릭터에 따로 저장하지 않음
	
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "First Person")
	TObjectPtr<UStaticMeshComponent> _ViewItemMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recoil")
	TObjectPtr<UControlShakeComponent> _ControlShakeManager;

	// 장착되는 모든 총기의 공통 Actor 클래스다. 시작 무기를 의미하지않는다.
	// 실제 무기 Actor를 생성할 공통 Blueprint 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AWeaponActor> _WeaponActorClass;

	// 현재 장착된 무기
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AWeaponActor> _CurrentWeapon;

	// 서버가 생성할 실제 수류탄의 Blueprint 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TSubclassOf<AGrenadeActor> _GrenadeActorClass;
	
	// 유효하면 현재 수류탄을 장착한 상태
	UPROPERTY(ReplicatedUsing = OnRep_CurrentGrenade, VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<AGrenadeActor> _CurrentGrenade;
	
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
	// Commit 건영 : 기존 FireOnce()를 반복 호출하여 연사했지만 FPSFireAbility가 담당으로 바뀜
	// FTimerHandle _FireTimerHandle;
	FTimerHandle _FiringTagTimerHandle; // 마지막 발사 이후 발사 표시를 종료하는 타이머

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

	AWeaponActor* GetEquippedWeapon() const;
	
	AGrenadeActor* GetEquippedGrenade() const;

	// 1인칭 GameplayCue를 뷰 무기 총구에 붙이기 위해 1인칭 무기 메시를 반환한다.
	UFUNCTION(BlueprintPure, Category = "First Person")
	USceneComponent* GetViewWeaponMeshComponent() const;
	
	// 투척할 액터는 보존하고 장착 참조와 표시만 정리
	void ClearGrenadeReference(AGrenadeActor* Grenade);
	
	UFUNCTION(Server, Reliable)
	void ServerEquipGrenade();
	
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
	
	// virtual void OnRep_PlayerState() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Jump() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Commit 건영 : AActor에 있는 함수인데 NearClip을 적용하기위해서 재정의해서 사용합니다.
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;
	
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
	USkeletalMeshComponent* Get_ViewWeaponMesh() const;

	// AbilityTask의 1인칭 표시 경로. 탄약/완료 판정은 서버 Ability에 남긴다.
	UFUNCTION(Client, Reliable)
	void ClientPlayMontage(EAnimMeshType Type, UAnimMontage* Montage, float PlayRate);
	void ClientPlayMontage_Implementation(EAnimMeshType Type, UAnimMontage* Montage, float PlayRate);

	UFUNCTION(Client, Reliable)
	void ClientStopMontage(EAnimMeshType Type, UAnimMontage* Montage);
	void ClientStopMontage_Implementation(EAnimMeshType Type, UAnimMontage* Montage);

	// 소유 플레이어의 1인칭 팔에서 재생하거나 지정 섹션으로 전환한다
	UFUNCTION(Client, Reliable)
	void ClientPlayGrenadeMontage(UAnimMontage* Montage, FName Section);
	UFUNCTION(Client, Reliable)
	void ClientStopGrenadeMontage(UAnimMontage* Montage);

	// ServerOnly 능력의 소유 클라이언트 TP 표시. 다른 클라이언트는 GAS 복제를 사용한다.
	UFUNCTION(Client, Reliable)
	void ClientPlayGrenadeMontageTP(UAnimMontage* Montage, FName Section);
	UFUNCTION(Client, Reliable)
	void ClientStopGrenadeMontageTP(UAnimMontage* Montage);
	
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

	UFUNCTION()
	void CookAction(const FInputActionValue& value);
	
	UFUNCTION()
	void EquipGrenadeAction(const FInputActionValue& value);
	
	UFUNCTION()
	void OnRep_CurrentGrenade();
	
	// 서버에서 장착을 해제하고 표시를 갱신
	void ClearEquippedGrenade();
	
	// Commit 건영 : 현재 발사 시작과 종료요청은 ASC의 SetFireInput() 경로가 담당한다.
	// UFUNCTION(Server, Reliable)
	// void ServerStartFire();
	// void ServerStartFire_Implementation();
	//
	// UFUNCTION(Server, Reliable)
	// void ServerStopFire();
	// void ServerStopFire_Implementation();

	// 소유 플레이어의 조준 상태를 서버 발사 계산에도 반영
	// UFUNCTION(Server, Reliable)
	// void ServerSetAiming(bool bAiming);

	
	// 서버에서 결정한 1인칭 외형을 소유 플레이어에게 전달하기 + 기존에는 무기 외형뿐이였는데 조준설정도 같이 받기
	UFUNCTION(Client, Reliable)
	//void ClientSetViewWeapon(USkeletalMesh* ViewMesh, TSubclassOf<UAnimInstance> ViewAnimationClass);
	void ClientSetViewWeapon(const FWeaponData& WeaponData);
	
	// 서버에서 실제 발사가 성공했을때만 호출
	UFUNCTION(Client, Reliable)
	void ClientWeaponFired(FName WeaponID);


	UFUNCTION()
	void DropItemAction(const FInputActionValue& value);

	UFUNCTION()
	void EquipMainWeaponAction(const FInputActionValue& value);

	UFUNCTION()
	void EquipSubWeaponAction(const FInputActionValue& value);

	// 지정 슬롯으로 교체 요청(클라 -> 서버)
	UFUNCTION(Server, Reliable)
	void ServerEquipSlot(int32 Index);
	void ServerEquipSlot_Implementation(int32 Index);


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
	
	// 두 입력이 공유하는 사전 검사 통과하면 서버에 요청함.
	void RequestEquipSlot(int32 Index);




// Commit 건영
// private:
// 	void ClearFiringTag(); -> 기존 StopFiringPresentation()에 태그 해제 처리까지 통합
// 	
// 	// 한 발의 조준점 계산과 발사를 실행
// 	void FireOnce();       -> FPSFireAbility::FireNextShot()에서 조준점을 구하고 무기 발사 호출 


};
