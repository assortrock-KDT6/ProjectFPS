#include "Character/CharacterPlayer.h"
#include "GameMode/FPSGameMode.h"
#include "GameMode/PlayerStateBase.h"
#include "Controller/PlayerControllerBase.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/InputSettings.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
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
#include "Component/Parkour/TraversalActionComponent.h"
#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "Component/Ability/GamePlayAbility/FPSMeleeAbility.h"
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
#include "Animation/WeaponAimSolver.h"
#include "HAL/IConsoleManager.h"
#include "GameTag/FPSGameplayTag.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"
#include "Net/UnrealNetwork.h"
#include "Settings/FPSGameUserSettings.h"



ACharacterPlayer::ACharacterPlayer(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// ASC 초기화 시 등록될 체력·실드 AttributeSet을 캐릭터마다 생성한다.
	_HealthAttribute = CreateDefaultSubobject<UFPSHealthSet>(TEXT("HealthAttributeSet"));

	USkeletalMeshComponent* MeshComp = GetMesh();
	
	if (nullptr != MeshComp)
	{
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		MeshComp->SetRelativeRotation(FVector(0.f, -90.f, 0.f).Rotation());
		// 화면을 렌더링하지 않는 서버에서도 서버 판정용 무기와 총구가 이 메시를 따라가도록 한다.
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
		MovementComp->GetNavAgentPropertiesRef().bCanCrouch = true;
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
	
	// 팔과 몸의 애니메이션 블루프린트 모두 Status.ADS를 GameplayTag_IsADS에 연결한다.
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
	if (IsValid(_CurrentGrenade))
	{
		return false;
	}

	// 소유 클라이언트의 장착 RPC는 무기 액터 참조 복제보다 먼저 도착할 수 있다.
	// 로컬 UI/탄퍼짐은 RPC로 받은 뷰 데이터를 기준으로 판단한다.
	// 서버와 비소유 프록시는 실제 무기 액터를 기준으로 유지한다.
	return GetLocalRole() == ROLE_AutonomousProxy
		? IsValid(_ViewWeaponData._ViewMesh)
		: IsValid(_CurrentWeapon);
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

	//const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_GameTraceChannel1, QueryParams);
	
	return bHit ? HitResult.ImpactPoint :TraceEnd;
}

FRotator ACharacterPlayer::GetWeaponAimRotation() const
{
	if (GetLocalRole() == ROLE_SimulatedProxy && true == _bHasReplicatedWeaponPoseAimPoint)
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		const double Age = true == IsValid(GameState)
			? GameState->GetServerWorldTimeSeconds() - _ReplicatedWeaponPoseAim._ServerTime : 0.0;
		// 관전자 표시만 최대 50ms 예측한다. 패킷 지연이 길어져도 계속 회전시키지 않는다.
		return WeaponAim::PredictRotation(_ReplicatedWeaponPoseAim._Rotation, _WeaponPoseAngularVelocity, Age);
	}
	// GetBaseAimRotation은 로컬 컨트롤러가 있으면 이전 카메라 캐시를 반환할 수 있다.
	return true == IsValid(GetController()) ? GetControlRotation() : GetBaseAimRotation();
}

FVector ACharacterPlayer::GetWeaponAimOrigin() const
{
	if (GetLocalRole() == ROLE_SimulatedProxy && true == _bHasReplicatedWeaponPoseAimPoint)
	{
		return _ReplicatedWeaponPoseAim._Origin;
	}
	if (false == IsValid(_CameraComponent) || false == IsValid(_SpringArmComponent))
	{
		return GetPawnViewLocation();
	}
	// 1인칭 팔의 평가 순서는 투척 노티파이에 필요하므로 바꾸지 않는다.
	// 현재 입력 회전으로 카메라의 상대 위치를 재구성하여 이전 프레임 회전을 제거한다.
	const FVector Anchor = _SpringArmComponent->GetComponentLocation();
	const FQuat ViewDelta = GetWeaponAimRotation().Quaternion() * _CameraComponent->GetComponentQuat().Inverse();
	return Anchor + ViewDelta.RotateVector(_CameraComponent->GetComponentLocation() - Anchor);
}

FVector ACharacterPlayer::TraceWeaponAimPoint(const FVector& Start, const FVector& Direction, float WeaponRange) const
{
	const FVector End = Start + Direction.GetSafeNormal() * WeaponRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponAim), false, this);
	FHitResult WorldHit;
	const bool WorldBlocked = GetWorld()->LineTraceSingleByChannel(WorldHit, Start, End, ECC_Visibility, Params);

	// Pawn 프로필은 Visibility를 무시한다. 카메라가 캐릭터를 겨눠도 뒤쪽 벽을
	// 조준점으로 삼지 않도록 Pawn 오브젝트를 별도로 검사하되, 앞의 벽을 통과하지 않는다.
	FCollisionObjectQueryParams PawnObjects;
	PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
	FHitResult PawnHit;
	const bool PawnBlocked = GetWorld()->LineTraceSingleByObjectType(PawnHit, Start, End, PawnObjects, Params);
	if (true == PawnBlocked && (false == WorldBlocked || PawnHit.Distance < WorldHit.Distance))
	{
		return PawnHit.ImpactPoint;
	}
	return true == WorldBlocked ? WorldHit.ImpactPoint : End;
}

FVector ACharacterPlayer::GetWeaponPoseAimPoint(float WeaponRange) const
{
	// 관전자도 서버 원점과 표시용 방향으로 가까운 충돌점을 구해 목표 경계의 깊이 변화를 반영한다.
	return TraceWeaponAimPoint(GetWeaponAimOrigin(), GetWeaponAimRotation().Vector(), WeaponRange);
}

void ACharacterPlayer::OnRep_WeaponPoseAimPoint()
{
	const double Interval = _ReplicatedWeaponPoseAim._ServerTime - _PreviousWeaponPoseServerTime;
	_WeaponPoseAngularVelocity = FRotator::ZeroRotator;
	if (true == _bHasReplicatedWeaponPoseAimPoint)
	{
		_WeaponPoseAngularVelocity = WeaponAim::AngularVelocity(
			_PreviousWeaponPoseRotation, _ReplicatedWeaponPoseAim._Rotation, Interval);
	}
	_PreviousWeaponPoseRotation = _ReplicatedWeaponPoseAim._Rotation;
	_PreviousWeaponPoseServerTime = _ReplicatedWeaponPoseAim._ServerTime;
	_bHasReplicatedWeaponPoseAimPoint = true;
	const IConsoleVariable* Diagnostics = IConsoleManager::Get().FindConsoleVariable(TEXT("fps.WeaponAim.Diagnostics"));
	if (nullptr != Diagnostics && Diagnostics->GetInt() != 0)
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		UE_LOG(LogTemp, Display, TEXT("WeaponAimReceive local=%.6f server=%.6f stamp=%.6f interval=%.6f yaw=%.6f velocity=%.6f"),
			GetWorld()->GetTimeSeconds(), true == IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : 0.0,
			_ReplicatedWeaponPoseAim._ServerTime, Interval, _ReplicatedWeaponPoseAim._Rotation.Yaw, _WeaponPoseAngularVelocity.Yaw);
	}
}

bool ACharacterPlayer::EquipWeapon(FName WeaponID)
{
	AWeaponActor* NewWeapon = PrepareWeapon(WeaponID);
	if (!IsValid(NewWeapon))
		return false;
	SaveCurrentWeaponToInventory();
	CommitPreparedWeapon(NewWeapon);
	return true;
}

// 생성·초기화·부착을 확정 전에 수행해, 교체 준비 실패 시 기존 무기를 보존한다.
// 반환된 액터는 숨겨진 준비 상태다. 호출부는 확정에 사용하거나 후속 준비 실패 시 Destroy해야 한다.
AWeaponActor* ACharacterPlayer::PrepareWeapon(FName WeaponID)
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
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponActor* NewWeapon = GetWorld()->SpawnActor<AWeaponActor>(_WeaponActorClass, GetActorTransform(), SpawnParameters);

	if (false == IsValid(NewWeapon))
	{
		return nullptr;
	}

	const bool bInitialized = IWeaponInterface::Execute_InitializeWeapon(NewWeapon, WeaponID);

	if (false == bInitialized)
	{
		NewWeapon->Destroy();
		return nullptr;
	}

	const FWeaponData& WeaponData = NewWeapon->GetWeaponData();

	if (!IsValid(WeaponData._ViewMesh))
	{
		NewWeapon->Destroy();
		return nullptr;
	}

	// 다른 플레이어에게 보이는 월드 무기는 소유 캐릭터의 3인칭 손에 부착된 상태로 네트워크 복제한다.
	// 로컬 플레이어의 1인칭 무기는 이와 분리하여, 1인칭 팔에 부착된 별도 컴포넌트로 관리한다.
	
	if (false == NewWeapon->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponGrip))
	{
		NewWeapon->Destroy();
		return nullptr;
	}

	// 라이플 메시의 총구는 +Y 방향이다. weapon_r 본에서 캐릭터 전방을
	// 향하도록 돌리고, 메시의 위쪽(+Z)은 그대로 유지한다.
	const FRotator WeaponRotation(0.f, 90.f, 0.f);
	const float WeaponScale = FMath::Clamp(WeaponData._ThirdPersonScale, 0.5f, 1.5f);
	const USceneComponent* WeaponMesh = NewWeapon->GetWeaponMeshComponent();
	const FVector RightGrip = true == WeaponMesh->DoesSocketExist(TEXT("RightHandGrip"))
		? WeaponMesh->GetSocketTransform(TEXT("RightHandGrip"), RTS_Component).GetLocation() : FVector::ZeroVector;
	// 오른손 그립을 축소 중심으로 사용하여 총 크기가 바뀌어도 방아쇠 손이 밀리지 않게 한다.
	const FVector GripPivotOffset = WeaponRotation.RotateVector(RightGrip * (1.f - WeaponScale));
	NewWeapon->SetActorRelativeTransform(FTransform(WeaponRotation, GripPivotOffset, FVector(WeaponScale)));

	NewWeapon->SetActorHiddenInGame(true);

	return NewWeapon;
}

// 줍기와 슬롯 전환이 같은 경로로 공격 중단, 액터 교체, 표시 갱신을 수행하도록 분리했다.
// 서버에서 무기 준비와 필요한 슬롯 반영이 끝난 뒤 호출한다. 기존 액터를 파괴하므로 잔탄 저장은 먼저 수행해야 한다.
void ACharacterPlayer::CommitPreparedWeapon(AWeaponActor* NewWeapon)
{
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
	NewWeapon->SetActorHiddenInGame(false);
	
	// 새 총 장착이 성공 -> 수류탄 장착을 해제하기
	if (IsValid(_CurrentGrenade))
	{
		ClearEquippedGrenade();
	}
	
	_bAiming = false;

	// 복제된 무기를 기준으로 1인칭 표시와 몸의 무기 자세를 함께 갱신한다.
	// 별도 클라이언트 RPC는 무기 참조나 데이터보다 먼저 도착하여 몸이 맨손 자세로 남을 수 있다.
	OnRep_CurrentWeapon();

	ForceNetUpdate();
}

void ACharacterPlayer::ClientSetViewWeapon_Implementation(const FWeaponData& WeaponData)
{
	_ViewWeaponData = WeaponData;
	_bPendingViewWeaponUpdate = true;
	ApplyPendingViewWeapon();
}

void ACharacterPlayer::ApplyPendingViewWeapon()
{
	if (!_bPendingViewWeaponUpdate || !IsLocallyControlled() || !IsValid(_ViewWeaponMesh))
	{
		return;
	}
	_bPendingViewWeaponUpdate = false;

	// 새 무기를 표시하기 전에 이전 총과 손의 반동을 제거
	if (IsValid(_ControlShakeManager))
	{
		_ControlShakeManager->ClearWeaponRecoil();
	}
	
	_ViewWeaponMesh->SetAnimInstanceClass(nullptr);
	
	_ViewWeaponMesh->SetSkeletalMesh(_ViewWeaponData._ViewMesh);

	if (_ViewWeaponData._ViewMesh && _ViewWeaponData._ViewAnimationInstance)
	{
		_ViewWeaponMesh->SetAnimInstanceClass(_ViewWeaponData._ViewAnimationInstance);
	}

	// 무기 교체 후에는 조준을 해제하고 기본 FOV로 돌아간다
	SetAiming(false);

	OnRep_CurrentGrenade();
}

void ACharacterPlayer::NotifyWeaponEquippedChanged()
{
	if (IsLocallyControlled())
	{
		WeaponEquppedChanged.Broadcast(IsWeaponEquipped());
	}
}

void ACharacterPlayer::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 리타기팅 원본과 표시용 손을 평가한 뒤 1인칭 투척 노티파이를 처리한다.
	// 스켈레탈 메시의 틱은 병렬 포즈 평가까지 끝난 뒤 완료된다.
	USkeletalMeshComponent* GripMesh = GetGrenadeGripMesh();
	_FirstPersonMesh->SetFOVFollower(_ViewItemMesh);
	// 카메라 기준으로 변형되는 1인칭 표시물은 월드 레이 트레이싱에서 제외한다.
	_ViewItemMesh->SetVisibleInRayTracing(false);
	if (IsValid(GripMesh) && IsValid(GetMesh()) && GripMesh != GetMesh())
	{
		GripMesh->AddTickPrerequisiteComponent(GetMesh());
	}
	if (IsValid(_FirstPersonMesh) && IsValid(GripMesh))
	{
		_FirstPersonMesh->AddTickPrerequisiteComponent(GripMesh);
	}

	// 파생 BP에 저장된 None 참조 대신 GAS가 실제 사용하는 체력 세트에 연결한다.
	// BP BeginPlay에서 피해가 발생하더라도 사망 이벤트를 놓치지 않도록 먼저 바인딩한다.
	UFPSHealthSet* HealthSet = IsValid(_AbilitySystemComponent)
		? const_cast<UFPSHealthSet*>(_AbilitySystemComponent->GetSet<UFPSHealthSet>()) : nullptr;
	_HealthAttribute = HealthSet;

	if (IsValid(HealthSet) && HasAuthority())
	{
		HealthSet->_OnOutOfHealth.AddUniqueDynamic(this, &ACharacterPlayer::HandleOutOfHealth);
	}
}

void ACharacterPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	if (true == IsValid(_CameraComponent))
	{
		_BaseCameraFOV   = _CameraComponent->FieldOfView;
		_TargetCameraFOV = _BaseCameraFOV;
	}

	if (true == IsDeathStarted())
	{
		ApplyDeathPresentation();
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
	UpdateGrenadeGripTick(false);
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
	
	if (true == _bAiming)
	{
		SetAiming(false);
	}
}

void ACharacterPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (true == IsDeathStarted())
	{
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		if (IsValid(Movement) && Movement->MovementMode != MOVE_None)
		{
			Movement->DisableMovement();
		}
		UpdateDeathMontage();
		return;
	}

	ApplyPendingViewWeapon();

	// 소유 클라이언트는 자신의 입력으로 즉시 조준하고, 관찰 클라이언트만 서버 값을 받는다.
	// 이 값은 표시용 포즈에만 사용하며 실제 사격·피해 계산 경로와 분리한다.
	if (true == HasAuthority() && true == IsValid(_CurrentWeapon) && false == IsValid(_CurrentGrenade))
	{
		_ReplicatedWeaponPoseAim._Origin = GetWeaponAimOrigin();
		_ReplicatedWeaponPoseAim._Rotation = GetWeaponAimRotation();
		_ReplicatedWeaponPoseAim._ServerTime = GetWorld()->GetTimeSeconds();
	}

	// 원격 플레이어도 복제된 몸 몽타주로 주먹 자세를 판단하여 무기를 숨긴다.
	const UFPSMeleeAbility* MeleeDefaults = true == IsValid(_AbilitySystemComponent)
		? _AbilitySystemComponent->MeleeAbilityClass.GetDefaultObject() : nullptr;
	UAnimInstance* BodyAnim = true == IsValid(GetMesh()) ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* BodyMontage = true == IsValid(BodyAnim) ? BodyAnim->GetCurrentActiveMontage() : nullptr;
	const bool HasBodyMeleePose = true == IsValid(MeleeDefaults) && true == MeleeDefaults->Attacks.ContainsByPredicate(
		[BodyMontage](const FFPSMeleeAttack& Attack)
		{
			return true == IsValid(BodyMontage) && Attack.ThirdPersonMontage == BodyMontage;
		});
	if (true == IsValid(_CurrentWeapon) && true == IsValid(_CurrentWeapon->GetRootComponent()))
	{
		const bool Visible = false == IsValid(_CurrentGrenade) && false == HasBodyMeleePose;
		if (_CurrentWeapon->GetRootComponent()->IsVisible() != Visible)
		{
			_CurrentWeapon->GetRootComponent()->SetVisibility(Visible, true);
		}
	}

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
		
		// 공격 상태가 해제되어도 팔 몽타주의 블렌딩이 끝날 때까지 1인칭 무기를 숨긴다.
		const bool HasMeleePose = (true == IsValid(_AbilitySystemComponent)
			&& true == _AbilitySystemComponent->HasMatchingGameplayTag(FPSGameplayTags::Status_Melee))
			|| (true == IsValid(MeleeDefaults) && true == MeleeDefaults->Attacks.ContainsByPredicate(
				[AnimInstance](const FFPSMeleeAttack& Attack)
				{
					return true == IsValid(AnimInstance) && true == IsValid(Attack.FirstPersonMontage)
						&& nullptr != AnimInstance->GetInstanceForMontage(Attack.FirstPersonMontage);
				}));
		const bool ShowWeapon = true == IsValid(_CurrentWeapon) && false == IsValid(_CurrentGrenade)
			&& false == HasGrenadePose && false == HasMeleePose;
		
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

	if (true == IsValid(_DefaultInput->_Melee))
	{
		InputComp->BindAction(_DefaultInput->_Melee, ETriggerEvent::Started,   this, &ACharacterPlayer::MeleeAction);
		InputComp->BindAction(_DefaultInput->_Melee, ETriggerEvent::Completed, this, &ACharacterPlayer::StopMeleeAction);
		InputComp->BindAction(_DefaultInput->_Melee, ETriggerEvent::Canceled,  this, &ACharacterPlayer::StopMeleeAction);
	}

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
	DOREPLIFETIME(ACharacterPlayer, _DeathStartServerTime);
	DOREPLIFETIME_CONDITION(ACharacterPlayer, _ReplicatedWeaponPoseAim, COND_SkipOwner);
}

// 해제 중 다른 총을 암묵적으로 장착하지 않도록 현재 총과 슬롯 정리만 담당한다.
// 다음 슬롯 자동 장착은 호출부(TryDropWeaponAt)가 결정하며, 수류탄 해제는 별도 경로다.
void ACharacterPlayer::ClearEquippedWeapon()
{
	if (!HasAuthority())
		return;
	SaveCurrentWeaponToInventory();
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

	_CurrentWeapon = nullptr;
	ClientSetViewWeapon(FWeaponData());

	OnRep_CurrentGrenade();
	APlayerStateBase* PS = GetPlayerState<APlayerStateBase>();
	if (IsValid(PS) && IsValid(PS->GetInventory()))
	{
		PS->GetInventory()->SetEquippedWEaponIndex(INDEX_NONE, false);
	}
	ForceNetUpdate();
}

// 새 무기를 줍거나 전환할 때 기존 액터의 잔탄이 유실되지 않도록 슬롯 저장을 공통화했다.
// 기존 액터 파괴 및 장착 슬롯 번호 변경 전에 호출한다. 버리기로 슬롯이 이미 제거됐다면 저장하지 않는다.
void ACharacterPlayer::SaveCurrentWeaponToInventory()
{
	if (!HasAuthority() || !IsValid(_CurrentWeapon))
	{
		return;
	}
	APlayerStateBase* PS = GetPlayerState<APlayerStateBase>();

	UInventoryComponent* Inv = IsValid(PS) ? PS->GetInventory() : nullptr;

	if (!IsValid(Inv))
	{
		return;
	}

	const int32 Index = Inv->GetEquippedWeaponIndex();

	const FWeaponSlotData* Slot = Inv->GetWeaponData(Index);

	if (!Slot || Slot->IsNone())
	{
		return;
	}

	FWeaponSlotData Data = *Slot;

	Data._CurrentAmmo = _CurrentWeapon->GetCurrentAmmo();

	Data._MaxAmmo = _CurrentWeapon->GetMaxAmmo();

	Inv->SetWeaponSlotData(Index, Data);
}

// 픽업 장착과 인벤토리 확정을 한곳에서 처리해 손의 무기와 슬롯 정보가 어긋나는 것을 방지한다.
// 새 무기와 밀려날 픽업 준비 -> 기존 잔탄 저장 -> 슬롯 확정 -> 장착 확정 -> 픽업·UI 마무리 순서다.
// 준비 실패 시 임시 액터만 제거하며, 교체로 떨어지는 무기에도 실제 잔탄을 전달한다.
bool ACharacterPlayer::TryAcquireWeapon(AWeaponPickUp* Pickup)
{
	if (!HasAuthority() || !IsValid(Pickup) || IsDeathStarted())
	{
		return false;
	}

	APlayerStateBase* PS = GetPlayerState<APlayerStateBase>();

	UInventoryComponent* Inv = IsValid(PS) ? PS->GetInventory() : nullptr;

	UTableSubsystem* Table = UTableSubsystem::Get(this);

	if (!IsValid(Inv) || !IsValid(Table))
	{
		return false;
	}

	const FName TID = Pickup->GetTID();

	const FItemData* Row = Table->FindTableRow<FItemData>(TEXT("ItemTable"), TID);

	int32 SlotIndex = INDEX_NONE;

	FName ReplacedTID = NAME_None;

	if (!Row || Row->_ItemType != EItemType::Weapon || Row->_WeaponId.IsNone()
		|| !Inv->FindEquipSlot(TID, SlotIndex, ReplacedTID))
	{
		return false;
	}

	AWeaponActor* NewWeapon = PrepareWeapon(Row->_WeaponId);
	if (!IsValid(NewWeapon))
	{
		return false;
	}

	NewWeapon->SetCurrentAmmo(Pickup->GetCurrentAmmo());

	// 모든 준비가 성공할 때까지 기존 액터와 슬롯을 유지한다.
	AWeaponPickUp* DroppedWeapon = nullptr;

	const FTransform DropTransform = Pickup->GetActorTransform();

	if (!ReplacedTID.IsNone())
	{
		AItemPickUp* PreparedPickup = AItemPickUp::BeginSpawnFromTID(GetWorld(), ReplacedTID, 1, DropTransform);

		DroppedWeapon = Cast<AWeaponPickUp>(PreparedPickup);

		if (!IsValid(DroppedWeapon))
		{
			if (IsValid(PreparedPickup))
			{
				PreparedPickup->Destroy();
			}
			NewWeapon->Destroy();
			return false;
		}

		const FWeaponSlotData* OldSlot = Inv->GetWeaponData(SlotIndex);

		const int32 OldAmmo = SlotIndex == Inv->GetEquippedWeaponIndex() && IsValid(_CurrentWeapon)
			? _CurrentWeapon->GetCurrentAmmo() : OldSlot->_CurrentAmmo;

		DroppedWeapon->SetCurrentAmmo(OldAmmo);
	}

	SaveCurrentWeaponToInventory();

	FWeaponSlotData Data;

	Data._WeaponId = TID;

	Data._CurrentAmmo = NewWeapon->GetCurrentAmmo();

	Data._MaxAmmo = NewWeapon->GetMaxAmmo();

	if (!Inv->CommitEquip(Data, SlotIndex, false))
	{
		NewWeapon->Destroy();
		if (IsValid(DroppedWeapon))
		{
			DroppedWeapon->Destroy();
		}
		return false;
	}

	CommitPreparedWeapon(NewWeapon);

	if (IsValid(DroppedWeapon))
	{
		AItemPickUp::FinishSpawnFromTID(DroppedWeapon, DropTransform);
	}

	Pickup->Destroy();

	Inv->NotifyWeaponsChanged();
	return true;
}

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

	AWeaponActor* NewWeapon = PrepareWeapon(Row->_WeaponId);
	if (!IsValid(NewWeapon))
		return false;

	SaveCurrentWeaponToInventory();

	NewWeapon->SetCurrentAmmo(WeaponSlotData->_CurrentAmmo);

	Inv->SetEquippedWEaponIndex(Index, false);

	CommitPreparedWeapon(NewWeapon);

	Inv->NotifyWeaponsChanged();

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

	const FWeaponSlotData* Slot = Inv->GetWeaponData(Index);
	const int32 CurrentAmmoData = bWasEquipped && IsValid(_CurrentWeapon)
		? _CurrentWeapon->GetCurrentAmmo() : Slot->_CurrentAmmo;
	AWeaponPickUp* WeaponPickUp = Cast<AWeaponPickUp>(Pickup);
	if (!IsValid(WeaponPickUp))
	{
		Pickup->Destroy();
		return false;
	}
	WeaponPickUp->SetCurrentAmmo(CurrentAmmoData);

	if (false == Inv->RemoveWeapon(Index, false))
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

	Inv->NotifyWeaponsChanged();
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
		float Sensitivity = _LookSensitivity;

		const UFPSGameUserSettings* Settings = UFPSGameUserSettings::GetFPSGameUserSettings();
		if (nullptr != Settings)
		{
			Sensitivity *= Settings->GetMouseSensitivity();
		}
		
		// 조준 중이면 무기 테이블의 조준 감도 배율을 추가로 곱한다.
		if (true == _bAiming)
		{
			Sensitivity *= _ViewWeaponData._AimSensitivityMultiplier;
		}

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
		// 맨손일 때는 일반 공격 입력을 근접 공격으로 전달한다.
		if (false == IsValid(_CurrentWeapon) && false == IsValid(_CurrentGrenade))
		{
			_AbilitySystemComponent->SetMeleeInput(true);
		}
		else
		{
			_AbilitySystemComponent->SetFireInput(true);
		}
	}
}

void ACharacterPlayer::StopFireAction(const FInputActionValue& value)
{
	if (IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->SetFireInput(false);
		// 맨손 공격으로 전달된 입력도 해제하여 다음 누름을 콤보 입력으로 받을 수 있게 한다.
		_AbilitySystemComponent->SetMeleeInput(false);
	}
}

void ACharacterPlayer::MeleeAction(const FInputActionValue& Value)
{
	if (true == IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->SetMeleeInput(true);
	}
}

void ACharacterPlayer::StopMeleeAction(const FInputActionValue& Value)
{
	if (true == IsValid(_AbilitySystemComponent))
	{
		_AbilitySystemComponent->SetMeleeInput(false);
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
	
	USkeletalMeshComponent* BodyMesh = GetGrenadeGripMesh();

	if (!IsValid(Inventory) || !IsValid(Table) || !IsValid(BodyMesh) || !BodyMesh->DoesSocketExist(TEXT("GrenadeGrip")))
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
	FTransform GripOffset = _GrenadeGripOffsetTP;
	GripOffset.SetScale3D(Grenade->GetActorRelativeScale3D() * GripOffset.GetScale3D());
	Grenade->SetActorRelativeTransform(GripOffset);

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
	UpdateGrenadeGripTick(IsValid(_CurrentGrenade) && !IsDeathStarted());
	if (true == IsDeathStarted())
	{
		ApplyDeathPresentation();
		return;
	}

	// UI 갱신은 아래 수류탄 연출의 조기 반환 여부와 무관하게 전달한다.
	NotifyWeaponEquippedChanged();

	const bool HasGrenade = IsValid(_CurrentGrenade);
	
	// 수류탄을 든 로컬 플레이어의 총과 손의 반동을 제거한다.
	if (IsLocallyControlled() && HasGrenade && IsValid(_ControlShakeManager))
	{
		_ControlShakeManager->ClearWeaponRecoil();
	}
	
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

				// 아이템의 피벗 보정을 유지하고, 1인칭 손 위치 보정은
				// 3인칭 액터의 부착 위치·회전과 독립적으로 적용한다.
				_ViewItemMesh->SetRelativeTransform(GrenadeMesh->GetRelativeTransform() * _GrenadeGripOffsetFP);

				for (int32 Index = 0; Index < GrenadeMesh->GetNumMaterials(); ++Index)
				{
					UMaterialInterface* Material = Index == 0 && IsValid(_GrenadeViewMaterial)
						? _GrenadeViewMaterial.Get() : GrenadeMesh->GetMaterial(Index);
					_ViewItemMesh->SetMaterial(Index, Material);
					_ViewItemMesh->CreateAndSetMaterialInstanceDynamic(Index);
				}
				_FirstPersonMesh->UpdateFOV();
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
		WeaponType = _CurrentWeapon->GetWeaponData()._WeaponType;
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

USkeletalMeshComponent* ACharacterPlayer::GetGrenadeGripMesh() const
{
	if (!_GrenadeGripMeshComponentName.IsNone())
	{
		TArray<USkeletalMeshComponent*> Meshes;
		GetComponents(Meshes);
		for (USkeletalMeshComponent* GripMesh : Meshes)
		{
			if (IsValid(GripMesh) && GripMesh->GetFName() == _GrenadeGripMeshComponentName && GripMesh->DoesSocketExist(TEXT("GrenadeGrip")))
			{
				return GripMesh;
			}
		}
	}
	return GetMesh();
}

FVector ACharacterPlayer::GetGrenadeReleaseLocation() const
{
	return IsValid(_CurrentGrenade) ? _CurrentGrenade->GetActorLocation() : GetActorLocation();
}

void ACharacterPlayer::UpdateGrenadeGripTick(bool Equipped)
{
	USkeletalMeshComponent* GripMesh = Equipped ? GetGrenadeGripMesh() : nullptr;
	if (_GrenadeTickMesh.Get() == GripMesh) return;
	if (USkeletalMeshComponent* Previous = _GrenadeTickMesh.Get())
	{
		Previous->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(_GrenadeSavedAnimTickOption);
		Previous->bEnableUpdateRateOptimizations = _GrenadeSavedUpdateRateOptimization;
		Previous->SetComponentTickEnabled(_GrenadeSavedTickEnabled);
	}
	_GrenadeTickMesh = GripMesh;
	if (IsValid(GripMesh))
	{
		_GrenadeSavedAnimTickOption = static_cast<uint8>(GripMesh->VisibilityBasedAnimTickOption);
		_GrenadeSavedUpdateRateOptimization = GripMesh->bEnableUpdateRateOptimizations;
		_GrenadeSavedTickEnabled = GripMesh->IsComponentTickEnabled();
		GripMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		GripMesh->bEnableUpdateRateOptimizations = false;
		GripMesh->SetComponentTickEnabled(true);
	}
}

USceneComponent* ACharacterPlayer::GetViewWeaponMeshComponent() const
{
	return _ViewWeaponMesh;
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
	if (true == IsDeathStarted())
	{
		return false;
	}

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

	// 서버의 탄 번호를 갱신
	// 호스트 자신의 총이라면 반동 연출도 처리
	if (IsValid(_ControlShakeManager))
	{
		_ControlShakeManager->WeaponFired(Weapon->GetWeaponData()._WeaponId);
	}
	
	// 원격 소유 클라이언트에 실제 발사 성공을 알린다.
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
	
	//	 서버에서 동일한 조건을 검사하고 있으므로 해당 줄은 주석처리하겠음. 
	//// 이미 그 슬롯을 들고 있으면 할 일 없음.
	//if (Index == Inv->GetEquippedWeaponIndex() && IsValid(_CurrentWeapon) && !IsValid(_CurrentGrenade))
	//	return;

	// 빈 슬롯이면 무시. _Weapons는 복제되니 클라에서도 안다.
	if (Inv->GetWeaponTID(Index).IsNone())
		return;

	ServerEquipSlot(Index);
}

void ACharacterPlayer::ClientWeaponFired_Implementation(FName WeaponID)
{
	// if (IsLocallyControlled() && IsValid(_ControlShakeManager)
	// 	&& IsValid(_AbilitySystemComponent) && _AbilitySystemComponent->CanAttack())
	// {
	// 	_ControlShakeManager->WeaponFired(WeaponID);
	// }
	// Commit 건영 : 호스트는 서버의 발사 성공 처리에서 이미 반동을 적용했음
	if (HasAuthority() || !IsLocallyControlled() || !IsValid(_ControlShakeManager))
	{
		return;
	}
	
	// 서버가 확정한 발사 결과이므로 현재 공격 가능 여부를 다시 검사하지 않음
	_ControlShakeManager->WeaponFired(WeaponID);
}

void ACharacterPlayer::ClientPlayMontage_Implementation(EAnimMeshType Type, UAnimMontage* Montage, float PlayRate)
{
	if (true == IsDeathStarted())
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = EAnimMeshType::FirstPerson == Type ? Get_FirstPersonMesh() : Get_ThirtPersonMesh();
	UAnimInstance* AnimInstance = IsValid(MeshComp) ? MeshComp->GetAnimInstance() : nullptr;
	if (!IsLocallyControlled() || !IsValid(AnimInstance) || !IsValid(Montage)
		|| !FMath::IsFinite(PlayRate) || PlayRate <= 0.f)
	{
		return;
	}
	AnimInstance->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, 0.f, false);
}

void ACharacterPlayer::ClientStopMontage_Implementation(EAnimMeshType Type,UAnimMontage* Montage)
{
	USkeletalMeshComponent* MeshComp = EAnimMeshType::FirstPerson == Type ? Get_FirstPersonMesh() : Get_ThirtPersonMesh();
	UAnimInstance* AnimInstance = IsValid(MeshComp) ? MeshComp->GetAnimInstance() : nullptr;
	if (IsLocallyControlled() && IsValid(AnimInstance) && IsValid(Montage))
	{
		AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
	}
}

void ACharacterPlayer::ClientPlayGrenadeMontage_Implementation(UAnimMontage* Montage, FName Section)
{
	if (true == IsDeathStarted() || false == IsLocallyControlled() || false == IsValid(_FirstPersonMesh) || false == IsValid(Montage))
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
	if (true == IsDeathStarted())
	{
		return;
	}

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
	RequestEquipSlot(0);
}

void ACharacterPlayer::EquipSubWeaponAction(const FInputActionValue& value)
{
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
	if (true == IsDeathStarted() || true == _AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}

	GameMode->HandlePlayerDeath(this, KillerPlayerState);
}

void ACharacterPlayer::BeginDeath()
{
	if (false == HasAuthority() || true == IsDeathStarted() || false == IsValid(GetWorld()))
	{
		return;
	}

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	_DeathStartServerTime = IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();

	// Ability 취소 중 추가 피해가 들어와도 재진입하지 않도록 사망 시각부터 기록한다.
	StopCombat();
	SetAnimationStateTag(FPSGameplayTags::Status_Death_Dead, true);
	ApplyDeathPresentation();
	ForceNetUpdate();
}

bool ACharacterPlayer::IsDeathStarted() const
{
	return _DeathStartServerTime >= 0.0;
}

void ACharacterPlayer::OnRep_DeathStartServerTime()
{
	if (true == IsDeathStarted())
	{
		ApplyDeathPresentation();
	}
}

void ACharacterPlayer::ApplyDeathPresentation()
{
	_bAiming = false;
	_bPendingViewWeaponUpdate = false;
	_TargetCameraFOV = _BaseCameraFOV;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (true == IsValid(Movement))
	{
		UFPSCharacterMovementComponent* FPSMovement = Cast<UFPSCharacterMovementComponent>(Movement);
		if (true == IsValid(FPSMovement))
		{
			FPSMovement->NotifyTraversalEnded();
		}
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
		Movement->SetComponentTickEnabled(false);
	}
	SetActorEnableCollision(false);

	TArray<UTraversalActionComponent*> TraversalActions;
	GetComponents(TraversalActions);
	for (UTraversalActionComponent* Action : TraversalActions)
	{
		if (true == IsValid(Action))
		{
			Action->ExitPresentation();
		}
	}
	if (true == IsValid(_ControlShakeManager))
	{
		_ControlShakeManager->ClearWeaponRecoil();
	}

	if (true == IsValid(_FirstPersonMesh))
	{
		UAnimInstance* AnimInstance = _FirstPersonMesh->GetAnimInstance();
		if (true == IsValid(AnimInstance))
		{
			AnimInstance->StopAllMontages(0.f);
		}
	}
	// 메시 표시 설정은 생존 중 상태를 유지한다. 별도 Death 카메라에서는
	// 기존 OwnerNoSee / OnlyOwnerSee 설정으로 전신과 1인칭 메시를 구분한다.
	UpdateDeathMontage();
}

void ACharacterPlayer::UpdateDeathMontage()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	UAnimInstance* AnimInstance = IsValid(BodyMesh) ? BodyMesh->GetAnimInstance() : nullptr;
	if (false == IsValid(AnimInstance))
	{
		return;
	}

	// 사망 이후 늦게 도착한 전투 몽타주 복제도 정리한다.
	for (const FAnimMontageInstance* Instance : AnimInstance->MontageInstances)
	{
		if (nullptr != Instance && Instance->IsActive() && Instance->Montage != _DeathMontage)
		{
			AnimInstance->StopAllMontages(0.f);
			break;
		}
	}
	if (false == IsValid(_DeathMontage) || false == IsValid(GetWorld())
		|| false == FMath::IsFinite(_DeathMontage->GetPlayLength()) || _DeathMontage->GetPlayLength() <= 0.f
		|| false == FMath::IsFinite(_DeathMontage->RateScale) || _DeathMontage->RateScale <= 0.f
		|| nullptr != AnimInstance->GetInstanceForMontage(_DeathMontage))
	{
		return;
	}

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const double ServerTime = IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	const float Position = FMath::Clamp(static_cast<float>((ServerTime - _DeathStartServerTime) * _DeathMontage->RateScale),
		0.f, FMath::Max(0.f, _DeathMontage->GetPlayLength() - KINDA_SMALL_NUMBER));
	if (AnimInstance->Montage_Play(_DeathMontage, 1.f, EMontagePlayReturnType::MontageLength, Position) <= 0.f)
	{
		return;
	}
	FAnimMontageInstance* Instance = AnimInstance->GetInstanceForMontage(_DeathMontage);
	if (nullptr != Instance)
	{
		// 공유 에셋은 수정하지 않고 이 시체의 재생 인스턴스만 마지막 포즈에 유지한다.
		Instance->bEnableAutoBlendOut = false;
		Instance->PushDisableRootMotion();
		for (int32 Index = 0; Index < _DeathMontage->CompositeSections.Num(); ++Index)
		{
			const FName NextSection = _DeathMontage->CompositeSections.IsValidIndex(Index + 1)
				? _DeathMontage->GetSectionName(Index + 1) : NAME_None;
			Instance->SetNextSectionName(_DeathMontage->GetSectionName(Index), NextSection);
		}
	}
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
	if (true == IsDeathStarted())
	{
		return;
	}

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
	if (Index == Inv->GetEquippedWeaponIndex() && IsValid(_CurrentWeapon) && !IsValid(_CurrentGrenade))
	{
		return;
	}

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
	if (_ObservedWeaponData.Get() != _CurrentWeapon.Get())
	{
		AWeaponActor* Previous = _ObservedWeaponData.Get();
		if (nullptr != Previous)
		{
			Previous->_OnWeaponDataChanged.RemoveDynamic(this, &ACharacterPlayer::OnRep_CurrentWeapon);
		}

		_ObservedWeaponData = _CurrentWeapon;
		if (true == IsValid(_CurrentWeapon))
		{
			_CurrentWeapon->_OnWeaponDataChanged.AddUniqueDynamic(this, &ACharacterPlayer::OnRep_CurrentWeapon);
		}
	}

	// WeaponID가 CurrentWeapon보다 늦게 복제되면 데이터 준비 알림에서 이 경로를 다시 실행한다.
	if (true == IsLocallyControlled()
		&& (false == IsValid(_CurrentWeapon) || true == _CurrentWeapon->GetWeaponData().IsValid()))
	{
		// ApplyPendingViewWeapon에서 몸의 애니메이션 레이어와 UI도 함께 갱신한다.
		ClientSetViewWeapon_Implementation(true == IsValid(_CurrentWeapon)
			? _CurrentWeapon->GetWeaponData() : FWeaponData());
		return;
	}

	// 장착 해제도 같은 경로로 UI에 알리고, 수류탄을 든 상태도 함께 반영한다.
	OnRep_CurrentGrenade();
}

FVector2D ACharacterPlayer::GetLookInput() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	
	// 로컬 플레이어의 이 허용된 경우에만 조회
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController() || PlayerController->IsLookInputIgnored() || !IsValid(_DefaultInput) || !IsValid(_DefaultInput->_MouseLook))
	{
		return FVector2D::ZeroVector;
	}
	
	const UEnhancedPlayerInput* PlayerInput = Cast<UEnhancedPlayerInput>(PlayerController->PlayerInput);
	
	if (!IsValid(PlayerInput))
	{
		return FVector2D::ZeroVector;
	}
	
	const FInputActionInstance* Action = PlayerInput->FindActionInstanceData(_DefaultInput->_MouseLook);
	
	// 기존 MoveLookAction과 동일한 발등 상태의 입력을 사용
	if (Action == nullptr || Action->GetTriggerEvent() != ETriggerEvent::Triggered)
	{
		return FVector2D::ZeroVector;
	}
	
	FVector2D Aim = Action->GetValue().Get<FVector2D>();
	
	const float Sensitivity = _LookSensitivity * (_bAiming ? _ViewWeaponData._AimSensitivityMultiplier : 1.0f);
	
	// 실제 시선 조작과 같은 부호화 감도를 적용
	Aim.X *= -Sensitivity;
	Aim.Y *= Sensitivity;
	
	// 컨트롤러의 추가 입력 배율을 반영
	if (GetDefault<UInputSettings>()->bEnableLegacyInputScales)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Aim.X *= PlayerController->GetDeprecatedInputYawScale();
		Aim.Y *= PlayerController->GetDeprecatedInputPitchScale();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	// 위에서 사용한 DEPRECATED API 는 현재 프로젝트가 활성화한 Legacy배율을 읽기 위해서 사용
	// 경고 억제 범위도 이 두 호출로 제한함
	
	// 반등과 카메라 각도 제한이 적용되기 전의 사용자 회전 입력
	return Aim;
	
	/* Commit 건영
	 * 목적	    : 발사 반동이 섞이지 않은 시선 입력을 제공
	 * 사용 위치 : AnimBP의 게임 스레드 입력 수집 단계
	 * 실행 흐름 : 입력 허용 확인 -> 액션 조회 -> 감도 와 컨트롤러 배율 적용 -> 반환
	 */
}
