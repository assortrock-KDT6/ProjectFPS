#include "Character/CharacterPlayer.h"
#include "GameMode/FPSGameMode.h"
#include "GameMode/PlayerStateBase.h"
#include "Controller/PlayerControllerBase.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Input/DefaultInput.h"
#include "Item/ItemPickUp.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Component/Parkour/VaultComponent.h"
#include "Component/Interaction/InteractionComponent.h"
#include "Component/Parkour/MantleComponent.h"
#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "Component/Ability/GamePlayAbility/FPSGrenadeAbility.h"
#include "UI/GameHUD.h"
#include "Projectiles/GrenadeActor.h"
#include "Weapons/Weaponactor.h"
#include "Weapons/WeaponPickUp.h"
#include "Weapons/WeaponInterface.h"
#include "Component/FOV/FPSViewSkeletalMeshComponent.h"
#include "Component/FOV/ControlShakeComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameTag/FPSGameplayTag.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"
#include "Net/UnrealNetwork.h"



ACharacterPlayer::ACharacterPlayer(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	USkeletalMeshComponent* MeshComp = GetMesh();
	
	if (nullptr != MeshComp)
	{
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		MeshComp->SetRelativeRotation(FVector(0.f, -90.f, 0.f).Rotation());
		// The authoritative weapon/muzzle follows this mesh, including on a headless server.
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	_SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SprintArm"));
	_CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	if (IsValid(CapsuleComp))
	{
		_SpringArmComponent->SetupAttachment(CapsuleComp);
		_SpringArmComponent->TargetArmLength = 0.f;
		_SpringArmComponent->SetRelativeRotation(FRotator::ZeroRotator);
		_SpringArmComponent->bUsePawnControlRotation = true;	// Camera와 1인칭 메시가 같은 ControlRotation을 따라가게 하기
		_SpringArmComponent->bInheritPitch           = true;
		_SpringArmComponent->bInheritYaw             = true;
		_SpringArmComponent->bInheritRoll            = false;
		_SpringArmComponent->bDoCollisionTest		 = false;	// SpringArmComponent의 길이가 0 인 1인칭 시점에서 SpringArm의 카메라 충돌 보정이 개입하지 않게 하는 설정
		_LookSensitivity							 = 0.75f;
	}

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (IsValid(MovementComp))
	{
		MovementComp->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}
	
	_FirstPersonMesh  = CreateDefaultSubobject<UFPSViewSkeletalMeshComponent>(TEXT("FirstPersonMesh"));	
	_FirstPersonMesh -> SetupAttachment(_SpringArmComponent);
	_FirstPersonMesh -> SetOnlyOwnerSee(true);
	_FirstPersonMesh -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_FirstPersonMesh -> SetGenerateOverlapEvents(false);
	_FirstPersonMesh -> SetCastShadow(false);
	_FirstPersonMesh -> VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones; // 서버에서도 Arm Bone 에 붙은 발사 Actor의 위치를 갱신해야한다.
	
	// 시점 회전은 부모 SpringArm에서 받고, 팔의 Camera 본을 따라간다.
	_CameraComponent -> SetupAttachment(_FirstPersonMesh, TEXT("camera"));
	_CameraComponent -> SetRelativeLocation(FVector::ZeroVector);
	_CameraComponent -> SetRelativeRotation(FRotator::ZeroRotator);
	_CameraComponent -> bUsePawnControlRotation = false;
	
	MeshComp		 -> SetOwnerNoSee(true);
	
	_ViewWeaponMesh = CreateDefaultSubobject<UFPSViewSkeletalMeshComponent>(TEXT("ViewWeaponMesh"));
	_ViewWeaponMesh->SetupAttachment(_FirstPersonMesh,TEXT("weapon"));
	_ViewWeaponMesh->SetRelativeLocation(FVector::ZeroVector);
	_ViewWeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
	_ViewWeaponMesh->SetRelativeScale3D(FVector::OneVector);
	
	// 표시용 아이템은 1인칭 오른손에 부착되어 움직이기
	_ViewItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewItemMesh"));
	_ViewItemMesh->SetupAttachment(_FirstPersonMesh, TEXT("GrenadeGrip"));
	_ViewItemMesh->SetRelativeLocation(FVector::ZeroVector);
	_ViewItemMesh->SetRelativeRotation(FRotator::ZeroRotator);
	_ViewItemMesh->SetRelativeScale3D(FVector::OneVector);

	// 소유 플레이어에게만 표시하며, 충돌은 실제 아이템 Actor가 담당한다.
	_ViewItemMesh->SetOnlyOwnerSee(true);
	_ViewItemMesh->SetCastShadow(false);
	_ViewItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_ViewItemMesh->SetGenerateOverlapEvents(false);
	
	_ControlShakeManager = CreateDefaultSubobject<UControlShakeComponent>(TEXT("ControlShakeComponent"));
	
	_HurdleCheckComponent = CreateDefaultSubobject<UHurdleCheckComponent>(TEXT("HurdleCheckComponent"));
	_VaultComponent = CreateDefaultSubobject<UVaultComponent>(TEXT("VaultComponent"));
	_MantleComponent = CreateDefaultSubobject<UMantleComponent>(TEXT("MantleComponent"));
	_InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

void ACharacterPlayer::SetAiming(bool bAiming)
{
	if (!IsLocallyControlled() || !IsValid(_CameraComponent))
	{
		return;
	}
	
	// 소유 플레이어가 전달받은 무기 설정으로 조준 가능 여부를 판단한다.
	_bAiming = bAiming && !IsValid(_CurrentGrenade) && IsValid(_ViewWeaponData._ViewMesh) && _ViewWeaponData._CanAim;

	// Commit 건영 : 중복코드 
	// if (!HasAuthority())
	// {
	// 	ServerSetAiming(_bAiming);
	// }
	
	// 기본 FOV * 배율 + 추가값
	_TargetCameraFOV = _bAiming ? FMath::Clamp(_BaseCameraFOV * _ViewWeaponData._AimFOVMultiplier + _ViewWeaponData._AimFOVAdditive, 1.f, 179.f) : _BaseCameraFOV;
	
	// 팔과 총기의 표시용 FOV도 같이 전환
	if (IsValid(_FirstPersonMesh))
	{
		_FirstPersonMesh -> SetTargetHFOV(_bAiming ? _ViewWeaponData._AimViewFOV : -1.f, 10.f);
	}
	
	if (IsValid(_ViewWeaponMesh))
	{
		_ViewWeaponMesh->SetTargetHFOV((_bAiming ? _ViewWeaponData._AimViewFOV : -1.f), 10.f);;
	}
	
	// Both the arms and body AnimBPs map Status.ADS to GameplayTag_IsADS.
	if (true == HasAuthority())
	{
		SetAnimationStateTag(FPSGameplayTags::Status_ADS, _bAiming);
	}
	else
	{
		ServerSetAiming(_bAiming);
	}
}

void ACharacterPlayer::ServerSetAiming_Implementation(bool bAiming)
{
	// 서버가 가진 실제 무기 설정으로 조준 가능 여부를 확인한다.
	_bAiming = bAiming && !IsValid(_CurrentGrenade) && IsValid(_CurrentWeapon) && _CurrentWeapon->GetWeaponData()._CanAim;
	
	// Commit 건영 : 코드 추가
	// 서버 조준 상태와 ADS 태그의 불일치를 방지하는 목적
	// Line 138~140 : 서버 권한이 있을떄 ADS 태그를 갱신하는데 클라이언트에서는 Line 144 : ServerSetAiming() 호출로 이 함수에 들어와요
	// 기존에는 여기서 _bAiming만 변경하고 ADS 태그는 갱신안했는데 서버가 검증한 최종 조준 상태를 ADS 태그에도 반영하게 했어요
	SetAnimationStateTag(FPSGameplayTags::Status_ADS, _bAiming);
}

bool ACharacterPlayer::IsWeaponEquipped() const
{
	//return IsValid(_CurrentWeapon);
	// Commit 건영 : 수류탄 작업중... 현재 이 함수가 총기 크로스헤어와 탄퍼짐 계산에 사용되고 있으니까 총 장착 판정으로 유지시키기
	//return HasAuthority() ? IsValid(_CurrentWeapon) : IsValid(_ViewWeaponData._ViewMesh);
	return !IsValid(_CurrentGrenade) && IsValid(_CurrentWeapon) && (HasAuthority() || IsValid(_ViewWeaponData._ViewMesh));
}

float ACharacterPlayer::GetWeaponSpreadValue() const
{
	if (!IsWeaponEquipped())
	{
		return 0.f;
	}
	
	// 서버는 실제 무기 데이터를, 소유 클라이언트는 전달받은 데이터를 사용
	const FWeaponData& WeaponData = HasAuthority() ? _CurrentWeapon->GetWeaponData() : _ViewWeaponData;
	
	const FWeaponSpreadInfo& SpreadInfo = WeaponData._SpreadInformation;
	
	// 조준과 공중상태는 지정된 값을 바로 반환하기
	if (_bAiming)
	{
		return SpreadInfo.ADS;
	}
	
	// 여기서 오류뜨면 #include "GameFramework/CharacterMovementComponent.h" 추가하면 됩니다. 기존 MovementComponent를 FPS 전용으로 따로 뺴두셨길래 혹시 몰라서 안넣었어요
	if (GetCharacterMovement()->IsFalling())
	{
		return SpreadInfo.Fall;
	}
	
	float Spread = bIsCrouched ? SpreadInfo.Crouch : SpreadInfo.Hip;
	
	// 속도 제곱 100은 실제 속도 10cm/s에 해당
	if (!bIsCrouched && GetVelocity().SizeSquared() >= 100.f)
	{
		Spread += SpreadInfo.Additive_Walk;
	}
	
	// 이미 관리하고 있는 무기별 발사 누적값을 재사용
	const int32 RecoilOffset = IsValid(_ControlShakeManager) ? _ControlShakeManager->GetRecoilOffset(WeaponData._WeaponId) : 0;
	
	Spread += FMath::Min(RecoilOffset, SpreadInfo.RecoilOffsetMax) * SpreadInfo.Additive_Recoil;
	
	return Spread;
}

FVector ACharacterPlayer::GetAimPoint(float WeaponRange) const
{
	const FVector CameraLocation = _CameraComponent->GetComponentLocation();

	// 현재 조준 방향을 중심으로, 탄퍼짐 각도 안에서 발사 방향을 뽑는다.
	const FVector FireDirection = FMath::VRandCone(GetControlRotation().Vector(), FMath::DegreesToRadians(GetWeaponSpreadValue()));
	
	const FVector TraceEnd = CameraLocation + FireDirection * WeaponRange;

	FHitResult HitResult;

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams);
	
	return bHit ? HitResult.ImpactPoint :TraceEnd;
}

bool ACharacterPlayer::EquipWeapon(FName WeaponID)
{
	USkeletalMeshComponent* BodyMesh = GetMesh();

	static const FName PreferredWeaponGrip(TEXT("weapon_r"));
	static const FName FallbackWeaponGrip(TEXT("hand_r"));
	const FName WeaponGrip = IsValid(BodyMesh) && BodyMesh->GetBoneIndex(PreferredWeaponGrip) != INDEX_NONE
		? PreferredWeaponGrip : FallbackWeaponGrip;

	if (false == HasAuthority()
		|| nullptr == _WeaponActorClass
		|| WeaponID.IsNone()
		|| false == IsValid(GetWorld())
		|| false == IsValid(BodyMesh)
		|| BodyMesh->GetBoneIndex(WeaponGrip) == INDEX_NONE)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponActor* NewWeapon = GetWorld()->SpawnActor<AWeaponActor>(_WeaponActorClass, GetActorTransform(), SpawnParameters);

	if (false == IsValid(NewWeapon))
	{
		return false;
	}

	const bool bInitialized = IWeaponInterface::Execute_InitializeWeapon(NewWeapon, WeaponID);

	if (false == bInitialized)
	{
		NewWeapon->Destroy();
		return false;
	}

	const FWeaponData& WeaponData = NewWeapon->GetWeaponData();

	if (!IsValid(WeaponData._ViewMesh))
	{
		NewWeapon->Destroy();
		return false;
	}

	// 다른 플레이어에게 보이는 월드 무기는 소유 캐릭터의 3인칭 손에 부착된 상태로 네트워크 복제한다.
	// 로컬 플레이어의 1인칭 무기는 이와 분리하여, 1인칭 팔에 부착된 별도 컴포넌트로 관리한다.
	
	if (false == NewWeapon->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponGrip))
	{
		NewWeapon->Destroy();
		return false;
	}

	// 라이플 메시의 총구는 +Y 방향이다. weapon_r 본에서 캐릭터 전방을
	// 향하도록 돌리고, 메시의 위쪽(+Z)은 그대로 유지한다.
	NewWeapon->GetRootComponent()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.f, 90.f, 0.f));

	// 무기를 바꾸면서 이전 무기의 연사가 이어지지 않게 한다.
	StopAttacking();
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->CancelWeaponReload();
	}

	_bAiming = false;

	SetAnimationStateTag(FPSGameplayTags::Status_ADS, false);

	if (true == IsValid(_CurrentWeapon))
	{
		_CurrentWeapon->Destroy();
	}

	_CurrentWeapon = NewWeapon;
	
	// 새 총 장착이 성공 -> 수류탄 장착을 해제하기
	if (IsValid(_CurrentGrenade))
	{
		ClearEquippedGrenade();
	}
	
	_bAiming = false;

	// 서버 쪽 기존 애니메이션 이벤트를 유지한다.
	// 로컬 플레이어의 이벤트는 아래 Client 함수에서 한 번 실행한다.
	if (!IsLocallyControlled())
	{
		OnWeaponEquiped(WeaponData._WeaponType);
	}
	
	//ClientSetViewWeapon(WeaponData._ViewMesh, WeaponData._ViewAnimationInstance);
	ClientSetViewWeapon(WeaponData);
	
	
	return true;
}

void ACharacterPlayer::ClientSetViewWeapon_Implementation(const FWeaponData& WeaponData)
{
	if (false == IsLocallyControlled() || false == IsValid(_ViewWeaponMesh))
	{
		return;
	}

	_ViewWeaponData = WeaponData;

	_ViewWeaponMesh->SetAnimInstanceClass(nullptr);
	
	_ViewWeaponMesh->SetSkeletalMesh(_ViewWeaponData._ViewMesh);

	if (_ViewWeaponData._ViewMesh && _ViewWeaponData._ViewAnimationInstance)
	{
		_ViewWeaponMesh->SetAnimInstanceClass(_ViewWeaponData._ViewAnimationInstance);
	}

	// 무기 교체 후에는 조준을 해제하고 기본 FOV로 돌아간다
	SetAiming(false);

	OnRep_CurrentGrenade();
	// OnWeaponEquiped(_ViewWeaponData._WeaponType);
	//
	// WeaponEquppedChanged.Broadcast(IsValid(_ViewWeaponData._ViewMesh));
}

void ACharacterPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	if (true == IsValid(_CameraComponent))
	{
		_BaseCameraFOV   = _CameraComponent->FieldOfView;
		_TargetCameraFOV = _BaseCameraFOV;
	}

	UFPSHealthSet* HealAttribute = Cast<UFPSHealthSet>(_HealthAttribute);

	if (true == HasAuthority() && IsValid(HealAttribute))
	{
		HealAttribute->_OnOutOfHealth.AddUniqueDynamic(this, &ACharacterPlayer::HandleOutOfHealth);
	}
}

// Commit 건영 : 부모호출 뒤에 같은 초기화만 반복함 자식만의 추가 구현이 없으므로 그냥 부모에서 끝나는게 맞음
// void ACharacterPlayer::OnRep_PlayerState()
// {
// 	Super::OnRep_PlayerState();
//
// 	if (nullptr != _AbilitySystemComponent)
// 	{
// 		_AbilitySystemComponent->InitAbilityActorInfo(this, this);
// 	}
// }

void ACharacterPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAttacking();
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->CancelWeaponReload();
	}

	if (true == HasAuthority())
	{
		if (true == IsValid(_CurrentWeapon))
		{
			_CurrentWeapon->Destroy();
		}
		if (true == IsValid(_PendingWeapon))
		{
			_PendingWeapon->Destroy();
		}
		if (true == IsValid(_CurrentGrenade))
		{
			_CurrentGrenade->Destroy();
			_CurrentGrenade = nullptr;
		}
	}

	// Commit 건영 : 중복삭제 -> 함수 첫줄 StopAttacking()이 아래의 역할을 함 
	// GetWorldTimerManager().ClearTimer(_FiringTagTimerHandle);

	UFPSHealthSet* HealthAttribute = Cast<UFPSHealthSet>(_HealthAttribute);

	if (true == IsValid(HealthAttribute))
	{
		HealthAttribute->_OnOutOfHealth.RemoveDynamic(this, &ACharacterPlayer::HandleOutOfHealth);
	}

	Super::EndPlay(EndPlayReason);
}

void ACharacterPlayer::Jump()
{
	Super::Jump();
}

void ACharacterPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (false == IsLocallyControlled() || !IsValid(_CameraComponent))
	{
		return;
	}
	
	_CameraComponent->SetFieldOfView(FMath::FInterpTo(_CameraComponent->FieldOfView, _TargetCameraFOV, DeltaTime, 10.f));
	
	if (IsValid(_ViewWeaponMesh))
	{
		const UFPSGrenadeAbility* GrenadeAbility = IsValid(_AbilitySystemComponent) ? _AbilitySystemComponent->GrenadeAbilityClass.GetDefaultObject() : nullptr;
		
		UAnimMontage* Montage = IsValid(GrenadeAbility) ? GrenadeAbility->GetGrenadeMontageFP() : nullptr;
		
		UAnimInstance* AnimInstance = IsValid(_FirstPersonMesh) ? _FirstPersonMesh->GetAnimInstance() : nullptr;
		
		const FAnimMontageInstance* MontageInstance = IsValid(Montage) && IsValid(AnimInstance) ? AnimInstance->GetInstanceForMontage(Montage) : nullptr;
		
		// 이전 Throw가 끝나면 다음 무기 장착과 Idle 애니메이션 준비
		if (IsValid(_CurrentGrenade) && IsValid(Montage) && IsValid(AnimInstance) && (MontageInstance == nullptr || (MontageInstance->GetCurrentSection() != FName(TEXT("Throw")) && IsValid(_ViewItemMesh) && _ViewItemMesh->GetStaticMesh() == nullptr)))
		{
			OnRep_CurrentGrenade();
		}
		
		// Blend Out 중에도 수류탄 동작이 남아 있으면 총을 그리지 않기
		const bool HasGrenadePose = IsValid(Montage) && IsValid(AnimInstance) && AnimInstance->GetInstanceForMontage(Montage) != nullptr;
		
		const bool ShowWeapon = IsValid(_CurrentWeapon) && !IsValid(_CurrentGrenade) && !HasGrenadePose;
		
		if (_ViewWeaponMesh->IsVisible() != ShowWeapon)
		{
			_ViewWeaponMesh->SetVisibility(ShowWeapon, true);
		}
	}
}

void ACharacterPlayer::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);
	
	// Commit 건영 : 코드 추가
	// 조준 중이고 테이블값이 양수라면 해당 무기의 NearClip 값을 사용
	// 조준 해제 또는 테이블 값이 0이하인 경우네는 -1로 지정
	// 이전 무기의 값이나 조준 중 값이 남지 않게하기
	OutResult.PerspectiveNearClipPlane = (_bAiming && _ViewWeaponData._AimNearClip > 0.f) ? _ViewWeaponData._AimNearClip : -1.f;
}

void ACharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* InputComp = Cast< UEnhancedInputComponent>(PlayerInputComponent);

	if (false == IsValid(InputComp))
	{
		return;
	}
	
	APlayerControllerBase* PlayerController = Cast<APlayerControllerBase>(GetController());

	if (false == IsValid(PlayerController))
	{
		return;
	}

	_DefaultInput = NewObject<UDefaultInput>(this);

	InputComp->BindAction(_DefaultInput->_Move,       ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveAction);
	InputComp->BindAction(_DefaultInput->_Jump,       ETriggerEvent::Triggered, this, &ACharacterPlayer::Jump);
	InputComp->BindAction(_DefaultInput->_MouseLook,  ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveLookAction);
	InputComp->BindAction(_DefaultInput->_AimZoom,    ETriggerEvent::Started,   this, &ACharacterPlayer::AimZoomAction);
	InputComp->BindAction(_DefaultInput->_Parkour,    ETriggerEvent::Started,   this, &ACharacterPlayer::ParkourAction);
	InputComp->BindAction(_DefaultInput->_Inventory,  ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleInventoryAction);
	InputComp->BindAction(_DefaultInput->_Map,		 ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleMapAction);
	InputComp->BindAction(_DefaultInput->_Interact,   ETriggerEvent::Started,   this, &ACharacterPlayer::InteractAction);
	InputComp->BindAction(_DefaultInput->_Fire,       ETriggerEvent::Started,   this, &ACharacterPlayer::FireAction);
	InputComp->BindAction(_DefaultInput->_Fire,       ETriggerEvent::Completed, this, &ACharacterPlayer::StopFireAction);
	InputComp->BindAction(_DefaultInput->_Fire,       ETriggerEvent::Canceled,  this, &ACharacterPlayer::StopFireAction);
	InputComp->BindAction(_DefaultInput->_FireToggle, ETriggerEvent::Started,   this, &ACharacterPlayer::FireToggleAction);
	InputComp->BindAction(_DefaultInput->_DropItem, ETriggerEvent::Started, this, &ACharacterPlayer::DropItemAction);
	InputComp->BindAction(_DefaultInput->_EquipMainWeapon, ETriggerEvent::Started, this, &ACharacterPlayer::EquipMainWeaponAction);
	InputComp->BindAction(_DefaultInput->_EquipSubWeapon, ETriggerEvent::Started, this, &ACharacterPlayer::EquipSubWeaponAction);
	InputComp->BindAction(_DefaultInput->_Cook,        ETriggerEvent::Started,   this, &ACharacterPlayer::CookAction);
	InputComp->BindAction(_DefaultInput->_WeaponSlot3, ETriggerEvent::Started,   this, &ACharacterPlayer::EquipGrenadeAction);
	
	if (IsValid(_AbilitySystemComponent))
	{
		InputComp->BindAction(_DefaultInput->_Reload, ETriggerEvent::Started, _AbilitySystemComponent.Get(), &UFPSAbilitySystemComponent::Reload);
	}

	PlayerController->RefreshInputMappingContext();
}

void ACharacterPlayer::PossessedBy(AController* Newcontroller)
{
	Super::PossessedBy(Newcontroller);

	_CombatPlayerState = GetPlayerState<APlayerStateBase>();

	// Commit 건영 : 부모에서 이미 호출하고 있는 초기화를 자식에서 중복으로 호출해서 삭제
	// 서버에서 ActorInfo 초기화
	// if (nullptr != _AbilitySystemComponent)
	// {
	// 	_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	// }
}

void ACharacterPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACharacterPlayer, _CurrentWeapon);
	DOREPLIFETIME(ACharacterPlayer, _CurrentGrenade);
}

void ACharacterPlayer::ClearEquippedWeapon()
{
	// 연사 중이면 끊는다
	StopAttacking();
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->CancelWeaponReload();
	}

	_bAiming = false;

	SetAnimationStateTag(FPSGameplayTags::Status_ADS, false);

	if (IsValid(_CurrentWeapon))
	{
		_CurrentWeapon->Destroy();
	}

	_CurrentWeapon = _PendingWeapon;

	_PendingWeapon = nullptr;

	if (true == IsValid(_CurrentWeapon))
	{
		OnWeaponEquiped(_CurrentWeapon->GetWeaponData()._WeaponType);
	}
	else
	{
		// 빈손 -> 1인칭 뷰 무기도 삭제 -> 액터만 지우면 화면에는 그대로 남음.
		ClientSetViewWeapon(FWeaponData());
	}
	
	OnRep_CurrentGrenade();
}

/**
 * 이렇게 했을 때 탄환이 보존 안됨. 수정하겠습니다. 
 */

bool ACharacterPlayer::TryEquipSlot(int32 Index)
{
	// 서버 확인
	if (false == HasAuthority())
	{
		return false;
	}

	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return false;

	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
		return false;

	// 지점 슬롯에 들어 있는 아이템 TID 확인.
	const FWeaponSlotData* WeaponSlotData = Inv->GetWeaponData(Index);

	if (nullptr == WeaponSlotData)
	{
		return false;
	}

	FName WeaponID = WeaponSlotData->_WeaponId;

	if (WeaponID.IsNone())
	{
		return false;
	}

	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return false;

	// 아이템 정보에서 무기 ID 확인.
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), WeaponID);

	if (nullptr == Row)
		return false;

	if (Row->_WeaponId.IsNone())
		return false;

	if (nullptr != _CurrentWeapon)
	{
		int32 PrevWeaponIndex		= Inv->GetEquippedWeaponIndex();

		FName PrevWeaponTID			= Inv->GetWeaponTID(PrevWeaponIndex);

		int32 PrevWeaponCurrentAmmo = _CurrentWeapon->GetCurrentAmmo();

		int32 PrevWeaponMaxAmmo		= _CurrentWeapon->GetMaxAmmo();

		FWeaponSlotData PrevWeaponSlotData;

		PrevWeaponSlotData._WeaponId	= PrevWeaponTID;

		PrevWeaponSlotData._CurrentAmmo = PrevWeaponCurrentAmmo;

		PrevWeaponSlotData._MaxAmmo		= PrevWeaponMaxAmmo;

		Inv->SetWeaponSlotData(PrevWeaponIndex, PrevWeaponSlotData);
	}

	// 손에 들 새 무기 액터 준비
	if (false == EquipWeapon(Row->_WeaponId))
		return false;

	_CurrentWeapon->SetCurrentAmmo(WeaponSlotData->_CurrentAmmo);

	// 실제 장착에 맞춰 인벤토리의 장착 슬롯 번호 기록.
	Inv->SetEquippedWEaponIndex(Index);

	return true;

}

bool ACharacterPlayer::TryDropWeaponAt(int32 Index)
{
	if (false == HasAuthority())
	{
		return false;
	}

	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
	{
		return false;
	}

	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
	{
		return false;
	}

	// 검증
	const FName TID = Inv->GetWeaponTID(Index);
	if (TID.IsNone())
	{
		return false;
	}

	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
	{
		return false; 
	}

	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);
	if (nullptr == Row)
	{
		return false;
	}

	// 픽업 준비 
	FTransform Transform = GetActorTransform();
	Transform.SetLocation(GetActorLocation() + GetActorForwardVector() * _DropForwardOffset);

	AItemPickUp* Pickup = AItemPickUp::BeginSpawnFromTID(GetWorld(), TID, 1, Transform);
	if (nullptr == Pickup)
	{
		return false;
	}

	// 인벤 확정 실패시 준비한 픽업 정리.
	const bool bWasEquipped = (Index == Inv->GetEquippedWeaponIndex());
	if (false == Inv->RemoveWeapon(Index))
	{
		Pickup->Destroy();
		return false;
	}

	// 손 확정시 
	if (bWasEquipped)
	{
		ClearEquippedWeapon();

		const int32 NextSlot = Inv->FindFirstWeaponSlot();
		if (NextSlot != INDEX_NONE)
		{
			TryEquipSlot(NextSlot);
		}
	}

	// 픽업
	AItemPickUp::FinishSpawnFromTID(Pickup, Transform);
	return true;
}


USkeletalMeshComponent* ACharacterPlayer::Get_FirstPersonMesh() const
{
	return _FirstPersonMesh;
}

USkeletalMeshComponent* ACharacterPlayer::Get_ThirtPersonMesh() const
{
	return GetMesh();
}

void ACharacterPlayer::MoveAction(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();

	AController* CurrentController = GetController();
	if (!IsValid(CurrentController))
	{
		return;
	}

	const FRotator Rotation		= CurrentController->GetControlRotation();
	const FRotator YawRotation	= FRotator(0.f, Rotation.Yaw, 0.f);

	FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector Right	= FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.X);
	AddMovementInput(Right, Axis.Y);
}

void ACharacterPlayer::MoveLookAction(const FInputActionValue& Value)
{
	// FVector2D Aim = Value.Get<FVector2D>();
	//
	// if (Controller)
	// {
	// 	AddControllerYawInput(-Aim.X * _LookSensitivity);
	// 	AddControllerPitchInput(Aim.Y * _LookSensitivity);
	// }
	
	// 조준할때 감도가 바뀌는데 기존에는 없어서 수정했어요 - 건영
	const FVector2D Aim = Value.Get<FVector2D>();
	
	if (Controller)
	{
		// 기본 감도는 유지, 실제 입력에 조준 배율 적용
		const float Sensitivity = _LookSensitivity * (_bAiming ? _ViewWeaponData._AimSensitivityMultiplier : 1.0f);
		
		AddControllerYawInput(-Aim.X * Sensitivity);
		
		AddControllerPitchInput(Aim.Y * Sensitivity);
	}
}

void ACharacterPlayer::AimZoomAction(const FInputActionValue& Value)
{
	SetAiming(!_bAiming);
	
	// 줌 상태일때 테이블에서 정해놓은 값들로 카메라값 변경
	
}

void ACharacterPlayer::ParkourAction(const FInputActionValue& Value)
{
	UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(GetCharacterMovement());
	if (false == IsValid(Movement))
	{
		return;
	}

	Movement->RequestTraversal();
}

void ACharacterPlayer::ToggleInventoryAction(const FInputActionValue& value)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AGameHUD* HUD = Cast<AGameHUD>(PC->GetHUD()))
			HUD->ToggleInventory();
	}

}

void ACharacterPlayer::ToggleMapAction(const FInputActionValue& value)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AGameHUD* HUD = Cast<AGameHUD>(PC->GetHUD()))
			HUD->ToggleMap();
		
	}
}

void ACharacterPlayer::SetNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp)
{
	if (IsValid(WeaponPickUp))
	{
		_NearbyWeaponPickUp = WeaponPickUp;
	}
}

void ACharacterPlayer::ClearNearbyWeaponPickUp(AWeaponPickUp* WeaponPickUp)
{
	if (_NearbyWeaponPickUp == WeaponPickUp)
	{
		_NearbyWeaponPickUp = nullptr;
	}
}

void ACharacterPlayer::InteractAction(const FInputActionValue& value)
{
	if (false == IsValid(_InteractionComponent))
	{
		return;
	}

	// 범위 내에 무기가 없으면 기존 작성되었던 Ray형식의 상호작용 방식을 사용하기
	_InteractionComponent->PickUpInteract();
}

void ACharacterPlayer::FireAction(const FInputActionValue& Value)
{
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->SetFireInput(true);
	}
}

void ACharacterPlayer::StopFireAction(const FInputActionValue& value)
{
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->SetFireInput(false);
	}
}

void ACharacterPlayer::FireToggleAction(const FInputActionValue& value)
{
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->ChangeFireMode();
	}
}

void ACharacterPlayer::CookAction(const FInputActionValue& value)
{
	if (IsValid(_AbilitySystemComponent)) // Commit 건영 : 에러나면 true == IsValid 형식으로 써봐요 Codex가 신박한 코드치길래 제맘대로 바꿨어요 
	{
		_AbilitySystemComponent->CookGrenade();
	}
}

void ACharacterPlayer::EquipGrenadeAction(const FInputActionValue& Value)
{
	if (IsLocallyControlled())
	{
		ServerEquipGrenade();
	}
}

void ACharacterPlayer::ServerEquipGrenade_Implementation()
{
	if (!HasAuthority() || IsValid(_CurrentGrenade) || nullptr == _GrenadeActorClass || !IsValid(GetWorld()))
	{
		return;
	}

	if (!IsValid(_AbilitySystemComponent) || _AbilitySystemComponent->HasMatchingGameplayTag(FPSGameplayTags::Status_Death))
	{
		return;
	}

	APlayerStateBase* FPSPlayerState = GetPlayerState<APlayerStateBase>();
	
	UInventoryComponent* Inventory = IsValid(FPSPlayerState) ? FPSPlayerState->GetInventory() : nullptr;

	UTableSubsystem* Table = UTableSubsystem::Get(this);
	
	USkeletalMeshComponent* BodyMesh = GetMesh();

	if (!IsValid(Inventory) || !IsValid(Table) || !IsValid(BodyMesh)  || BodyMesh->GetBoneIndex(TEXT("hand_r")) == INDEX_NONE)
	{
		return;
	}

	const AGrenadeActor* Defaults = _GrenadeActorClass.GetDefaultObject();
	
	if (!IsValid(Defaults) || Defaults->GetTID().IsNone())
	{
		return;
	}

	const FName TID = Defaults->GetTID();

	// 클라이언트의 주장 대신 서버 인벤토리의 실제 수량을 확인한다.
	bool HasGrenade = false;

	for (const FInventorySlot& Item : Inventory->GetItems())
	{
		if (Item._TID == TID && Item._Count > 0)
		{
			HasGrenade = true;
			
			break;
		}
	}

	if (!HasGrenade)
	{
		return;
	}

	const FItemData* ItemData = Table->FindTableRow<FItemData>(TEXT("ItemTable"), TID);

	const UStaticMeshComponent* DefaultMesh = Defaults->GetGrenadeMesh();

	if (nullptr == ItemData || ItemData->_ItemType != EItemType::Grenade || !IsValid(ItemData->_WorldMesh) || !IsValid(DefaultMesh) || DefaultMesh->GetStaticMesh() != ItemData->_WorldMesh)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGrenadeActor* Grenade = GetWorld()->SpawnActor<AGrenadeActor>(_GrenadeActorClass, GetActorTransform(), SpawnParameters);

	if (!IsValid(Grenade))
	{
		return;
	}

	if (!Grenade->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("GrenadeGrip")))
	{
		Grenade->Destroy();
		
		return;
	}

	// 새 수류탄 준비가 성공한 뒤 기존 총의 공격을 중단
	StopAttacking();
	_AbilitySystemComponent->CancelWeaponReload();
	
	_bAiming = false;
	
	SetAnimationStateTag(FPSGameplayTags::Status_ADS, false);

	_CurrentGrenade = Grenade;

	// C++ RepNotify는 서버에서 자동 호출되지 않으므로 직접 적용
	OnRep_CurrentGrenade();
	
	ForceNetUpdate();
}

void ACharacterPlayer::OnRep_CurrentGrenade()
{
	const bool HasGrenade = IsValid(_CurrentGrenade);
	
	const bool HasWeapon = IsValid(_CurrentWeapon);

	// 총 Actor는 보존하되 수류탄을 들고 있을 때 외형을 숨긴다.
	if (HasWeapon && IsValid(_CurrentWeapon->GetRootComponent()))
	{
		_CurrentWeapon->GetRootComponent()->SetVisibility(!HasGrenade, true);
	}

	// Commit 건영 : 수류탄 무기 전환 버그를 1인칭 총 표시를 Tick()에서 결정으로 해결
	// if (IsValid(_ViewWeaponMesh))
	// {
	// 	_ViewWeaponMesh->SetVisibility(IsLocallyControlled() && HasWeapon && !HasGrenade, true);
	// }

	if (IsLocallyControlled() && HasGrenade && IsValid(_AbilitySystemComponent) && IsValid(_FirstPersonMesh))
	{
		const UFPSGrenadeAbility* GrenadeAbility = _AbilitySystemComponent->GrenadeAbilityClass.GetDefaultObject();
		
		UAnimMontage* Montage = IsValid(GrenadeAbility) ? GrenadeAbility->GetGrenadeMontageFP() : nullptr;
		
		UAnimInstance* AnimInstance = _FirstPersonMesh->GetAnimInstance();
		
		const FAnimMontageInstance* MontageInstance = IsValid(Montage) && IsValid(AnimInstance) ? AnimInstance->GetInstanceForMontage(Montage) : nullptr;
		
		// 서버의 재장착이 먼저 도착해도 이전 Throw에 새 수류탄을 표시하지 않음
		if (MontageInstance && MontageInstance->GetCurrentSection() == FName(TEXT("Throw")))
		{
			if (IsValid(_ViewItemMesh))
			{
				_ViewItemMesh->SetStaticMesh(nullptr);
			}
			
			return;
		}
	}
	
	if (IsValid(_ViewItemMesh))
	{
		_ViewItemMesh->EmptyOverrideMaterials();
		
		_ViewItemMesh->SetStaticMesh(nullptr);

		if (IsLocallyControlled() && HasGrenade)
		{
			const UStaticMeshComponent* GrenadeMesh = _CurrentGrenade->GetGrenadeMesh();

			if (IsValid(GrenadeMesh))
			{
				_ViewItemMesh->SetStaticMesh(GrenadeMesh->GetStaticMesh());

				// Blueprint에서 맞춘 메시 원점 보정도 함께 적용한다.
				_ViewItemMesh->SetRelativeTransform(GrenadeMesh->GetRelativeTransform());

				for (int32 Index = 0; Index < GrenadeMesh->GetNumMaterials(); ++Index)
				{
					_ViewItemMesh->SetMaterial(Index, GrenadeMesh->GetMaterial(Index));
				}
			}
		}
	}

	EWeaponType WeaponType = EWeaponType::None;

	if (HasGrenade)
	{
		WeaponType = EWeaponType::Grenade;
	}
	else if (HasWeapon)
	{
		WeaponType = IsLocallyControlled() ? _ViewWeaponData._WeaponType : _CurrentWeapon->GetWeaponData()._WeaponType;
	}

	OnWeaponEquiped(WeaponType);

	// 복제된 장착 상태로 모든 TP 메시가 같은 첫 프레임을 유지한다.
	// 준비/투척 중인 GAS 몽타주는 장착 갱신으로 되돌리거나 중단하지 않는다.
	if (IsValid(_AbilitySystemComponent) && IsValid(GetMesh()))
	{
		const UFPSGrenadeAbility* GrenadeAbility = _AbilitySystemComponent->GrenadeAbilityClass.GetDefaultObject();
		UAnimMontage* MontageTP = IsValid(GrenadeAbility) ? GrenadeAbility->GetGrenadeMontageTP() : nullptr;
		UAnimInstance* AnimInstanceTP = GetMesh()->GetAnimInstance();
		if (IsValid(MontageTP) && IsValid(AnimInstanceTP))
		{
			if (HasGrenade)
			{
				if (!AnimInstanceTP->Montage_IsActive(MontageTP)
					&& AnimInstanceTP->Montage_Play(MontageTP, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false) > 0.f)
				{
					AnimInstanceTP->Montage_Pause(MontageTP);
				}
			}
			else if (AnimInstanceTP->Montage_IsActive(MontageTP) && !AnimInstanceTP->Montage_IsPlaying(MontageTP))
			{
				AnimInstanceTP->Montage_Stop(MontageTP->GetDefaultBlendOutTime(), MontageTP);
			}
		}
	}

	if (IsLocallyControlled())
	{
		SetAiming(false);
		
		WeaponEquppedChanged.Broadcast(IsWeaponEquipped());
	}
	
	if (IsLocallyControlled() &&
		IsValid(_AbilitySystemComponent) &&
		IsValid(_FirstPersonMesh))
	{
		const UFPSGrenadeAbility* GrenadeAbility = _AbilitySystemComponent->GrenadeAbilityClass.GetDefaultObject();
		
		UAnimMontage* Montage = IsValid(GrenadeAbility) ? GrenadeAbility->GetGrenadeMontageFP() : nullptr;

		UAnimInstance* AnimInstance = _FirstPersonMesh->GetAnimInstance();
		
		if (IsValid(Montage) && IsValid(AnimInstance))
		{
			if (HasGrenade)
			{
				// 이미 존재하면 Ready, Throw를 첫 프레임으로 되돌리지 않는다.
				if (!AnimInstance->Montage_IsActive(Montage) && AnimInstance->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false) > 0.f)
				{
					AnimInstance->Montage_Pause(Montage);
				}
			}
			else if (AnimInstance->Montage_IsActive(Montage) && !AnimInstance->Montage_IsPlaying(Montage))
			{
				// 장착해제 시 멈춰 있던 IDLE 표시를 정리한다.
				AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
			}
		}
	}
}

void ACharacterPlayer::ClearGrenadeReference(AGrenadeActor* Grenade)
{
	if (false == HasAuthority() || false == IsValid(Grenade) || _CurrentGrenade.Get() != Grenade)
	{
		return;
	}
	
	_CurrentGrenade = nullptr;
	
	OnRep_CurrentGrenade();
	
	ForceNetUpdate();
}

void ACharacterPlayer::ClearEquippedGrenade()
{
	if (false == HasAuthority())
	{
		return;
	}

	// 장착한 액터를 삭제하기 전에 준비 동작부터 취소
	if (IsValid(_AbilitySystemComponent) && nullptr != _AbilitySystemComponent->GrenadeAbilityClass)
	{
		if (FGameplayAbilitySpec* Spec = _AbilitySystemComponent->FindAbilitySpecFromClass(_AbilitySystemComponent->GrenadeAbilityClass))
		{
			if (Spec->IsActive())
			{
				_AbilitySystemComponent->CancelAbilityHandle(Spec->Handle);
			}
		}
	}

	AGrenadeActor* Grenade = _CurrentGrenade;
	
	_CurrentGrenade = nullptr;

	OnRep_CurrentGrenade();

	if (IsValid(Grenade))
	{
		Grenade->Destroy();
	}

	ForceNetUpdate();
}

AWeaponActor* ACharacterPlayer::GetEquippedWeapon() const
{
	return IsValid(_CurrentGrenade) ? nullptr : _CurrentWeapon.Get();
}

AGrenadeActor* ACharacterPlayer::GetEquippedGrenade() const
{
	return _CurrentGrenade.Get();
}

// Commit 건영 : 중복코드 제거
// void ACharacterPlayer::ServerStartFire_Implementation()
// {
// 	if (false == IsValid(_CurrentWeapon))
// 	{
// 		return;
// 	}
//
// 	// 버튼을 누른 순간 첫 발은 즉시 발사
// 	FireOnce();
//
// 	// 단발 무기는 첫 발 이후 반복 타이머를 시작하지 않는다.
// 	if (EWeaponFireMode::Automatic != _CurrentWeapon->GetFireMode())
// 	{
// 		return;
// 	}
//
// 	const float ProjectileInterval = _CurrentWeapon->GetProjectileInterval();
//
// 	if (ProjectileInterval <= 0.f)
// 	{
// 		return;
// 	}
//
// 	GetWorldTimerManager().SetTimer(_FireTimerHandle, this, &ACharacterPlayer::FireOnce, ProjectileInterval, true, ProjectileInterval);
// }
//
// void ACharacterPlayer::ServerStopFire_Implementation()
// {
// 	GetWorldTimerManager().ClearTimer(_FireTimerHandle);
// }
//
// void ACharacterPlayer::FireOnce()
// {
// 	if (false == HasAuthority() || false == IsValid(_CurrentWeapon))
// 	{
// 		GetWorldTimerManager().ClearTimer(_FireTimerHandle);
// 		return;
// 	}
//
// 	const FVector AimPoint = GetAimPoint(_CurrentWeapon->GetWeaponRange());
// 	
// 	// 실제 총알 생성에 성공했을때만 반동을 전달
// 	if (_CurrentWeapon->Fire(AimPoint))
// 	{
// 		ClientWeaponFired(_CurrentWeapon->GetWeaponData()._WeaponId);
// 	}
// }

// 일부는 태그 막는 걸로 처리할 수 있을 것 같음. 
// Movement 상태때문에 따로 함수로 빼놓음.
bool ACharacterPlayer::CanFireFromAbility() const
{
	const UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(GetCharacterMovement());

	if (false == IsValid(GetWorld()))
	{
		return false;
	}

	if (true == IsValid(_CurrentGrenade))
	{
		return false;
	}

	if (false == IsValid(_CurrentWeapon))
	{
		return false;
	}

	if (false == IsValid(Movement))
	{
		return false;
	}

	if (true == Movement->GetTraversalState().IsActive())
	{
		return false;
	}

	if (true == Movement->IsTraversing())
	{
		return false;
	}

	return true;
}

void ACharacterPlayer::NotifyAbilityWeaponFired(AWeaponActor* Weapon)
{

	if (false == HasAuthority() || false == IsValid(Weapon) || Weapon != _CurrentWeapon)
	{
		return;
	}
	
	SetAnimationStateTag(FPSGameplayTags::Status_Firing, true);
	
	GetWorldTimerManager().SetTimer(_FiringTagTimerHandle, this, &ACharacterPlayer::StopFiringPresentation, FMath::Max(0.1f, Weapon->GetProjectileInterval() + 0.05f), false);

	ClientWeaponFired(Weapon->GetWeaponData()._WeaponId);
}

void ACharacterPlayer::StopFiringPresentation()
{
	GetWorldTimerManager().ClearTimer(_FiringTagTimerHandle);
	
	SetAnimationStateTag(FPSGameplayTags::Status_Firing, false);
}

// Commit 건영 : 발사 표시를 끄는 코드는 기존 함수 하나로 모으고 중복을 제거
// void ACharacterPlayer::ClearFiringTag()
// {
// 	if (true == HasAuthority())
// 	{
// 		SetAnimationStateTag(FPSGameplayTags::Status_Firing, false);
// 	}
// }

void ACharacterPlayer::RequestEquipSlot(int32 Index)
{
	// UE_LOG(LogTemp, Warning, TEXT("[Equip] 요청 Index=%d"), Index);

	// 인벤, 맵이 열려 있으면 숫자키를 UI가 쓸 수 있으니 막는다.
	APlayerController* Pc = Cast<APlayerController>(GetController());
	if (nullptr != Pc)
	{
		AGameHUD* Hud = Cast<AGameHUD>(Pc->GetHUD());
		if (nullptr != Hud && Hud->IsAnyOverlayOpen())
			return;
	}

	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return;

	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
		return;

	// 이미 그 슬롯을 들고 있으면 할 일 없음.
	if (Index == Inv->GetEquippedWeaponIndex())
		return;

	// 빈 슬롯이면 무시. _Weapons는 복제되니 클라에서도 안다.
	if (Inv->GetWeaponTID(Index).IsNone())
		return;

	ServerEquipSlot(Index);
}

void ACharacterPlayer::ClientWeaponFired_Implementation(FName WeaponID)
{
	if (IsLocallyControlled() && IsValid(_ControlShakeManager)
		&& IsValid(_AbilitySystemComponent) && _AbilitySystemComponent->CanAttack())
	{
		_ControlShakeManager->WeaponFired(WeaponID);
	}
}

void ACharacterPlayer::ClientPlayGrenadeMontage_Implementation(UAnimMontage* Montage, FName Section)
{
	if (false == IsLocallyControlled() || false == IsValid(_FirstPersonMesh) || false == IsValid(Montage))
	{
		return;
	}
	
	if (Montage->GetSectionIndex(Section) == INDEX_NONE)
	{
		return;
	}
	
	UAnimInstance* AnimInstance = _FirstPersonMesh->GetAnimInstance();
	
	if (false == IsValid(AnimInstance))
	{
		return;
	}
	
	// 이미 재생중이면 다시 시작하지 않고 섹션만 전환한다.
	if (false == AnimInstance->Montage_IsActive(Montage))
	{
		const float Duration = AnimInstance->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false);
		
		if (Duration <= 0.f)
		{
			return;
		}
	}
	
	AnimInstance->Montage_JumpToSection(Section, Montage);
	AnimInstance->Montage_Resume(Montage);
}

void ACharacterPlayer::ClientStopGrenadeMontage_Implementation(UAnimMontage* Montage)
{
	if (false == IsLocallyControlled() || false == IsValid(_FirstPersonMesh) || false == IsValid(Montage))
	{
		return;
	}
	
	UAnimInstance* AnimInstance = _FirstPersonMesh->GetAnimInstance();
	
	if (false == IsValid(AnimInstance))
	{
		return;
	}
	
	AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
}

void ACharacterPlayer::ClientPlayGrenadeMontageTP_Implementation(UAnimMontage* Montage, FName Section)
{
	USkeletalMeshComponent* Body = GetMesh();
	UAnimInstance* AnimInstance = IsValid(Body) ? Body->GetAnimInstance() : nullptr;
	if (!IsLocallyControlled() || HasAuthority() || !IsValid(AnimInstance) || !IsValid(Montage)
		|| Montage->GetSectionIndex(Section) == INDEX_NONE)
	{
		return;
	}
	if (!AnimInstance->Montage_IsActive(Montage)
		&& AnimInstance->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false) <= 0.f)
	{
		return;
	}
	AnimInstance->Montage_JumpToSection(Section, Montage);
	AnimInstance->Montage_Resume(Montage);
}

void ACharacterPlayer::ClientStopGrenadeMontageTP_Implementation(UAnimMontage* Montage)
{
	USkeletalMeshComponent* Body = GetMesh();
	UAnimInstance* AnimInstance = IsValid(Body) ? Body->GetAnimInstance() : nullptr;
	if (IsLocallyControlled() && !HasAuthority() && IsValid(AnimInstance) && IsValid(Montage))
	{
		AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
	}
}

void ACharacterPlayer::DropItemAction(const FInputActionValue& value)
{
	// 인벤 / 맵이 열려 있으면 버리기를 막음.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AGameHUD* HUD = Cast<AGameHUD>(PC->GetHUD()))
		{
			if(HUD->IsAnyOverlayOpen())
			{
				return;
			}
		}
	}
	ServerDropEquippedWeapon();
}

/**
 * 이렇게 했을 때 매번 새로운 WeaponActor 가 생성되는 문제가 있음. => 탄환수 보존이 안됨.
 * Slot 에 현재 탄환수 저장하고 불러오게 해야할 듯.
 */
void ACharacterPlayer::EquipMainWeaponAction(const FInputActionValue& value)
{
	// UE_LOG(LogTemp, Warning, TEXT("[Equip] 입력 Main"));
	RequestEquipSlot(0);
}

void ACharacterPlayer::EquipSubWeaponAction(const FInputActionValue& value)
{
	// UE_LOG(LogTemp, Warning, TEXT("[Equip] 입력 Sub"));
	RequestEquipSlot(1);
}

void ACharacterPlayer::HandleOutOfHealth(APlayerStateBase* KillerPlayerState)
{
	AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();

	if (false == HasAuthority() || false == IsValid(GameMode) || false == GameMode->IsCombatAllowed())
	{
		return;
	}

	if (false == IsValid(_AbilitySystemComponent))
	{
		return;
	}
	
	const FGameplayTag DeadTag = FPSGameplayTags::Status_Death_Dead;

	// 중복 사망 처리 방지
	if (true == _AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}

	SetAnimationStateTag(DeadTag, true);

	GameMode->HandlePlayerDeath(this, KillerPlayerState);
}

void ACharacterPlayer::StopAttacking()
{
	// 발사 Ability는 서버에서 실행되니까 취소도 서버에서 처리하기
	if (HasAuthority() && IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->CancelWeaponFire();
	}

	StopFiringPresentation();
	
	SetAnimationStateTag(FPSGameplayTags::Status_Melee, false);
}

void ACharacterPlayer::StopCombat()
{
	if (false == HasAuthority())
	{
		return;
	}

	StopAttacking();

	_bAiming = false;

	SetAnimationStateTag(FPSGameplayTags::Status_ADS, false);
	if (IsLocallyControlled())
	{
		SetAiming(false);
	}
	SetAnimationStateTag(FPSGameplayTags::Status_Reloading, false);
	SetAnimationStateTag(FPSGameplayTags::Status_Dashing, false);

	// 진행 중인 사격, 재정전, 파쿠르 Ability 취소
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->CancelAllAbilities();
	}
}

APlayerStateBase* ACharacterPlayer::GetCombatPlayerState() const
{
	return _CombatPlayerState.Get();
}

void ACharacterPlayer::InitializeAfterRespawn()
{
	if (false == HasAuthority())
	{
		return;
	}

	UFPSHealthSet* HealthSet = Cast<UFPSHealthSet>(_HealthAttribute);

	if (true == IsValid(HealthSet))
	{
		HealthSet->Set_Health(HealthSet->Get_MaxHealth());
		HealthSet->Set_Shield(HealthSet->Get_MaxShield());
		HealthSet->Set_DamageIn(0.f);
	}

	SetAnimationStateTag(FPSGameplayTags::Status_Death_Dead, false);

	APlayerStateBase* FPSPlayerState = GetPlayerState<APlayerStateBase>();

	UInventoryComponent* Inventory = IsValid(FPSPlayerState) ? FPSPlayerState->GetInventory() : nullptr;

	if (true == IsValid(Inventory) && Inventory->GetEquippedWeaponIndex() != INDEX_NONE)
	{
		TryEquipSlot(Inventory->GetEquippedWeaponIndex());
	}
}

void ACharacterPlayer::FellOutOfWorld(const UDamageType& DamageType)
{
	const AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();

	if (HasAuthority() && IsValid(GameMode) && GameMode->IsCombatAllowed() && IsValid(GetController()))
	{
		HandleOutOfHealth(nullptr);
		return;
	}
	Super::FellOutOfWorld(DamageType);
}

void ACharacterPlayer::ServerEquipSlot_Implementation(int32 Index)
{
	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return;

	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
		return;

	// 서버 재 검증 
	if (Index == Inv->GetEquippedWeaponIndex())
		return;

	TryEquipSlot(Index);

}

void ACharacterPlayer::ServerDropEquippedWeapon_Implementation()
{
	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return;
	
	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
		return;

	if (IsValid(_CurrentGrenade))
	{
		const FName TID = _CurrentGrenade->GetTID();
		const TArray<FInventorySlot>& Items = Inv->GetItems();
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index]._TID == TID && Items[Index]._Count > 0)
			{
				TryDropItemAt(Index, 1);
				return;
			}
		}
		ClearEquippedGrenade();
		return;
	}
	TryDropWeaponAt(Inv->GetEquippedWeaponIndex());
}

void ACharacterPlayer::ServerDropWeaponAt_Implementation(int32 Index)
{
	TryDropWeaponAt(Index);
}



void ACharacterPlayer::ServerDropItemAt_Implementation(int32 Index, int32 Count)
{
	TryDropItemAt(Index, Count);
}

bool ACharacterPlayer::TryDropItemAt(int32 Index, int32 Count)
{
	if (false == HasAuthority())
		return false;
	
	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return false;

	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
		return false;

	// 검증 - 클라가 보낸 수량을 그대로 믿지않음.
	const TArray<FInventorySlot>& Items = Inv->GetItems();
	if (false == Items.IsValidIndex(Index))
		return false;

	const FName TID = Items[Index]._TID;
	const int32 DropCount = FMath::Clamp(Count, 0, Items[Index]._Count);
	if (TID.IsNone() || DropCount <= 0)
		return false; 

	// 픽업 준비 (Deferred)
	FTransform Transform = GetActorTransform();
	Transform.SetLocation(GetActorLocation() + GetActorForwardVector() * _DropForwardOffset);
	
	AItemPickUp* Pickup = AItemPickUp::BeginSpawnFromTID(GetWorld(), TID, DropCount, Transform);
	if (nullptr == Pickup)
		return false;

	// 인벤에서 빼기 -> 실패 시 준비한 픽업 정리.
	if (false == Inv->RemoveItem(Index, DropCount))
	{
		Pickup->Destroy();
		return false;
	}

	// 마지막 수류탄을 버렸다면 손에 남은 표시도 정리 -> 현재 RemoveItem 은 배열 요소를 삭제하지않고 해당 슬롯을 비우게 되어있음
	if (IsValid(_CurrentGrenade)&& _CurrentGrenade->GetTID() == TID && Inv->GetItems()[Index]._Count <= 0)
	{
		ClearEquippedGrenade();
	}
	
	// 픽업 등장
	AItemPickUp::FinishSpawnFromTID(Pickup, Transform);
	return true;

}

void ACharacterPlayer::OnRep_CurrentWeapon()
{
	if (false == IsValid(_CurrentWeapon))
	{
		// 무기를 버려 빈손이 됐다는 것도 알려야 한다.
		OnWeaponEquiped(EWeaponType::None);
		return;
	}

	OnWeaponEquiped(_CurrentWeapon->GetWeaponData()._WeaponType);
	// if (false == IsValid(_CurrentWeapon))
	// {
	// 	return;
	// }
	//
	// OnWeaponEquiped(_CurrentWeapon->GetWeaponData()._WeaponType);
	OnRep_CurrentGrenade();
}
