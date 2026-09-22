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
#include "Component/Inventory/InventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Component/Parkour/VaultComponent.h"
#include "Component/Interaction/InteractionComponent.h"
#include "Component/Parkour/MantleComponent.h"
#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "UI/GameHUD.h"
#include "Weapons/Weaponactor.h"
#include "Weapons/WeaponPickUp.h"
#include "Weapons/WeaponInterface.h"
#include "Component/FOV/FPSViewSkeletalMeshComponent.h"
#include "Component/FOV/ControlShakeComponent.h"
#include "Animation/AnimInstance.h"
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
	
	_ControlShakeManager = CreateDefaultSubobject<UControlShakeComponent>(TEXT("ControlShakeComponent"));
	
	_HurdleCheckComponent = CreateDefaultSubobject<UHurdleCheckComponent>(TEXT("HurdleCheckComponent"));
	_VaultComponent = CreateDefaultSubobject<UVaultComponent>(TEXT("VaultComponent"));
	_MantleComponent = CreateDefaultSubobject<UMantleComponent>(TEXT("MantleComponent"));
	_InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

void ACharacterPlayer::SetAiming(bool bAniming)
{
	if (HasAuthority() && IsValid(_AbilitySystemComponent))
	{
		SetAnimationStateTag(FPSGameplayTags::Status_ADS, bAniming && !_AbilitySystemComponent->HasMatchingGameplayTag(FPSGameplayTags::Status_Death));
	}
	else if (IsLocallyControlled())
	{
		ServerSetAiming(bAniming);
	}
}

void ACharacterPlayer::ServerSetAiming_Implementation(bool bAiming)
{
	SetAiming(bAiming);
}

void ACharacterPlayer::ClearFiringTag()
{
	SetAnimationStateTag(FPSGameplayTags::Status_Firing, false);
}

FVector ACharacterPlayer::GetAimPoint(float WeaponRange) const
{
	const FVector CameraLocation = _CameraComponent->GetComponentLocation();

	const FVector TraceEnd = CameraLocation + _CameraComponent->GetForwardVector() * WeaponRange;

	FHitResult HitResult;

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams);

	if (bHit)
	{
		return HitResult.ImpactPoint;
	}

	return TraceEnd;
}

bool ACharacterPlayer::EquipWeapon(FName WeaponID)
{
	if (!HasAuthority()
		|| !_WeaponActorClass
		|| WeaponID.IsNone()
		|| !IsValid(GetWorld())
		|| !IsValid(_FirstPersonMesh)
		|| _FirstPersonMesh->GetBoneIndex(TEXT("weapon")) == INDEX_NONE)
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

	// 기존 발사 Actor의 장착 위치는 유지한다.
	if (!NewWeapon->AttachToComponent(_FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon")))
	{
		NewWeapon->Destroy();
		return false;
	}

	// 무기를 바꾸면서 이전 무기의 연사가 이어지지 않게 한다.
	GetWorldTimerManager().ClearTimer(_FireTimerHandle);

	if (IsValid(_CurrentWeapon))
	{
		_CurrentWeapon->Destroy();
	}

	_CurrentWeapon = NewWeapon;

	// 서버 쪽 기존 애니메이션 이벤트를 유지한다.
	// 로컬 플레이어의 이벤트는 아래 Client 함수에서 한 번 실행한다.
	if (!IsLocallyControlled())
	{
		OnWeaponEquiped(WeaponData._WeaponType);
	}

	ClientSetViewWeapon(WeaponData._ViewMesh, WeaponData._ViewAnimationInstance);
	return true;
}
void ACharacterPlayer::ClientSetViewWeapon_Implementation(USkeletalMesh* ViewMesh, TSubclassOf<UAnimInstance> ViewAnimClass)
{
	if (!IsLocallyControlled() || !IsValid(_ViewWeaponMesh))
	{
		return;
	}

	// 이전 총기의 애니메이션 인스턴스를 먼저 정리한다.
	_ViewWeaponMesh->SetAnimInstanceClass(nullptr);
	_ViewWeaponMesh->SetSkeletalMesh(ViewMesh);

	if (ViewMesh && ViewAnimClass)
	{
		_ViewWeaponMesh->SetAnimInstanceClass(ViewAnimClass);
	}

	if (true == IsValid(_CurrentWeapon))
	{
		OnWeaponEquiped(_CurrentWeapon->GetWeaponData()._WeaponType);
	}
}

void ACharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	UFPSHealthSet* HealAttribute = Cast<UFPSHealthSet>(_HealthAttribute);

	if (true == HasAuthority() && IsValid(HealAttribute))
	{
		HealAttribute->_OnOutOfHealth.AddUniqueDynamic(this, &ACharacterPlayer::HandleOutOfHealth);
	}
}

void ACharacterPlayer::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (nullptr != _AbilitySystemComponent)
	{
		_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ACharacterPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(_FiringTagTimerHandle);
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

	//UEnhancedInputLocalPlayerSubsystem* Subsystem = 
	//	ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	//if (false == IsValid(Subsystem))
	//{
	//	return;
	//}

	//Subsystem->AddMappingContext(_DefaultInput->_DefaultInputMappingContext.Get(), 0);

	InputComp->BindAction(_DefaultInput->_Move,       ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveAction);
	InputComp->BindAction(_DefaultInput->_Jump,       ETriggerEvent::Triggered, this, &ACharacterPlayer::Jump);
	InputComp->BindAction(_DefaultInput->_MouseLook,  ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveLookAction);
	InputComp->BindAction(_DefaultInput->_AimZoom,    ETriggerEvent::Triggered, this, &ACharacterPlayer::AimZoomAction);
	InputComp->BindAction(_DefaultInput->_Parkour,    ETriggerEvent::Started,   this, &ACharacterPlayer::ParkourAction);
	InputComp->BindAction(_DefaultInput->_Inventory,  ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleInventoryAction);
	InputComp->BindAction(_DefaultInput->_Map,		 ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleMapAction);
	InputComp->BindAction(_DefaultInput->_Interact,   ETriggerEvent::Started,   this, &ACharacterPlayer::InteractAction);
	InputComp->BindAction(_DefaultInput->_Fire,       ETriggerEvent::Started,   this, &ACharacterPlayer::FireAction);
	InputComp->BindAction(_DefaultInput->_Fire,       ETriggerEvent::Completed, this, &ACharacterPlayer::StopFireAction);
	InputComp->BindAction(_DefaultInput->_FireToggle, ETriggerEvent::Started,   this, &ACharacterPlayer::FireToggleAction);
	InputComp->BindAction(_DefaultInput->_DropItem, ETriggerEvent::Started, this, &ACharacterPlayer::DropItemAction);
	PlayerController->RefreshInputMappingcontext();
}

void ACharacterPlayer::PossessedBy(AController* Newcontroller)
{
	Super::PossessedBy(Newcontroller);

	/**
	 * 서버에서 ActorInfo 초기화
	 */
	if (nullptr != _AbilitySystemComponent)
	{
		_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ACharacterPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACharacterPlayer, _CurrentWeapon);
}

void ACharacterPlayer::ClearEquippedWeapon()
{
	// 연사 중이면 끊는다
	GetWorldTimerManager().ClearTimer(_FireTimerHandle);

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
	const FName TID = Inv->GetWeaponTID(Index);
	if (TID.IsNone())
		return false;

	UTableSubsystem* Sub = UTableSubsystem::Get(this);
	if (nullptr == Sub)
		return false;

	// 아이템 정보에서 무기 ID 확인.
	const FItemData* Row = Sub->FindTableRow<FItemData>(TEXT("ItemTable"), TID);

	if (nullptr == Row)
		return false;

	if (Row->_WeaponId.IsNone())
		return false;

	// 손에 들 새 무기 액터 준비
	if (false == EquipWeapon(Row->_WeaponId))
		return false;

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
	FVector2D Aim = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(-Aim.X * _LookSensitivity);
		AddControllerPitchInput(Aim.Y * _LookSensitivity);
	}
}

void ACharacterPlayer::AimZoomAction(const FInputActionValue& Value)
{
	
	// Jisoo's Code : 조준으로 확대가 힘들다면 돋보기로 키우기
	// if (IsValid(_SpringArmComponent))
	// {
	// 	const float ZoomValue = Value.Get<float>() * _ZoomSensitivity;
	// 	const float Length = _SpringArmComponent->TargetArmLength;
	//
	// 	// [Todo] : 최대 거리, 최소 거리 변수로 분리할 것.
	// 	_SpringArmComponent->TargetArmLength = FMath::Clamp(_SpringArmComponent->TargetArmLength + ZoomValue, Length - 300.f, Length + 200.f);
	// }
}

void ACharacterPlayer::ParkourAction(const FInputActionValue& Value)
{
	// todo : 파쿠르 액션에 대한 판정은 C++ , 애니메이션과 세부 판정값은 Blueprint로 작성하기
	// 파쿠르 액션은 여러개의 LineTrace를 통해 해당 물체의 오브젝트의 크기를 알아내고 Vault 와 Mentle 액션을 결정한다.

	//const bool bVaultActive = IsValid(_ParkourComponent) && _ParkourComponent->IsVaultActive();
	//
	//const bool bMantleActive = IsValid(_MantleComponent) && _MantleComponent->IsMantleActive();
	//
	//if (bVaultActive || bMantleActive)
	//{
	//	return;
	//}
	//
	//if (IsValid(_ParkourComponent))
	//{
	//	_ParkourComponent->Server_TryParkour();
	//	if (true == _ParkourComponent->IsVaultActive())
	//	{
	//		return;
	//	}

	//}

	//if (IsValid(_MantleComponent))
	//{
	//	_MantleComponent->TryMantle();
	//	// _ParkourComponent->TryParkour(); <-- Fatal Bug FIX : 두개의 파쿠르 액션을 취하게 되니 Flying 상태에서 다시 Flying 상태가 되어 이전 상태인 Move_Walk를 기억하지못해 계속된 Flying 상태가 유지됐다.
	//	//  todo : 애니메이션 몽타쥬가 종료될때까지 액션이 끝날동안 추가 파쿠르 입력의 키는 받지 않게 설정한다.
	//}

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

void ACharacterPlayer::FireAction(const FInputActionValue& value)
{
	ServerStartFire();
}

void ACharacterPlayer::StopFireAction(const FInputActionValue& value)
{
	ServerStopFire();
}

void ACharacterPlayer::FireToggleAction(const FInputActionValue& value)
{
	ServerToggleFireMode();
}

void ACharacterPlayer::ServerStartFire_Implementation()
{
	if (false == IsValid(_CurrentWeapon) || true == _AbilitySystemComponent->HasMatchingGameplayTag(FPSGameplayTags::Status_Death))
	{
		return;
	}

	// 버튼을 누른 순간 첫 발은 즉시 발사
	FireOnce();

	// 단발 무기는 첫 발 이후 반복 타이머를 시작하지 않는다.
	if (EWeaponFireMode::Automatic != _CurrentWeapon->GetFireMode())
	{
		return;
	}

	const float ProjectileInterval = _CurrentWeapon->GetProjectileInterval();

	if (ProjectileInterval <= 0.f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(_FireTimerHandle, this, &ACharacterPlayer::FireOnce, ProjectileInterval, true, ProjectileInterval);
}

void ACharacterPlayer::ServerStopFire_Implementation()
{
	GetWorldTimerManager().ClearTimer(_FireTimerHandle);
}

void ACharacterPlayer::ServerToggleFireMode_Implementation()
{
	if (false == IsValid(_CurrentWeapon))
	{
		return;
	}

	// 발사 중 모드를 바꾸면 기존 연사 타이머부터 정지한다.
	GetWorldTimerManager().ClearTimer(_FireTimerHandle);

	_CurrentWeapon->ToggleFireMode();
}

void ACharacterPlayer::FireOnce()
{
	if (false == HasAuthority() || false == IsValid(_CurrentWeapon)
		|| _AbilitySystemComponent->HasMatchingGameplayTag(FPSGameplayTags::Status_Death))
	{
		GetWorldTimerManager().ClearTimer(_FireTimerHandle);
		return;
	}

	const FVector AimPoint = GetAimPoint(_CurrentWeapon->GetWeaponRange());
	
	// 실제 총알 생성에 성공했을때만 반동을 전달
	if (_CurrentWeapon->Fire(AimPoint))
	{
		SetAnimationStateTag(FPSGameplayTags::Status_Firing, true);
		GetWorldTimerManager().SetTimer(_FiringTagTimerHandle, this, &ACharacterPlayer::ClearFiringTag,
			FMath::Max(0.1f, _CurrentWeapon->GetProjectileInterval() + 0.05f), false);
		ClientWeaponFired(_CurrentWeapon->GetWeaponData()._WeaponId);
	}
}

void ACharacterPlayer::ClientWeaponFired_Implementation(FName WeaponID)
{
	if (IsLocallyControlled() && IsValid(_ControlShakeManager))
	{
		_ControlShakeManager->WeaponFired(WeaponID);
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

void ACharacterPlayer::HandleOutOfHealth()
{
	if (false == HasAuthority())
	{
		return;
	}

	if (false == IsValid(_AbilitySystemComponent))
	{
		return;
	}
	
	const FGameplayTag DeadTag = FPSGameplayTags::Status_Death_Dead;

	// 중복 사망 처리 방지
	if (_AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}

	SetAnimationStateTag(DeadTag, true);
	GetWorldTimerManager().ClearTimer(_FireTimerHandle);
	GetWorldTimerManager().ClearTimer(_FiringTagTimerHandle);
	ClearFiringTag();
	SetAiming(false);
	SetAnimationStateTag(FPSGameplayTags::Status_Reloading, false);
	SetAnimationStateTag(FPSGameplayTags::Status_Melee, false);
	SetAnimationStateTag(FPSGameplayTags::Status_Dashing, false);

	// 진행 중인 사격, 재정전, 파쿠르 Ability 취소
	_AbilitySystemComponent->CancelAllAbilities();

	AFPSGameMode* GameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();
	if (true == IsValid(GameMode))
	{
		GameMode->HandlePlayerDeath(this);
	}
}

void ACharacterPlayer::ServerDropEquippedWeapon_Implementation()
{
	APlayerStateBase* Ps = GetPlayerState<APlayerStateBase>();
	if (nullptr == Ps)
		return;
	
	UInventoryComponent* Inv = Ps->GetInventory();
	if (nullptr == Inv)
		return;

	TryDropWeaponAt(Inv->GetEquippedWeaponIndex());
}


void ACharacterPlayer::SetupPlayerMesh()
{
	if (false == IsLocallyControlled())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (false == IsValid(MeshComponent))
	{
		return;
	}

	// MeshComponent->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);

	// TODO
	// 플레이어 몸통은 마테리얼로 나누는 걸 추천.
	// MeshComponent->SetMaterial(TorsoMaterialIndex, InvisibleMaterial);
}

void ACharacterPlayer::OnRep_CurrentWeapon()
{
	if (false == IsValid(_CurrentWeapon))
	{
		return;
	}

	OnWeaponEquiped(_CurrentWeapon->GetWeaponData()._WeaponType);
}
