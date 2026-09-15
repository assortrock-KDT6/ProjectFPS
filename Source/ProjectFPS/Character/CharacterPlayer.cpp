#include "Character/CharacterPlayer.h"
#include "Controller/PlayerControllerBase.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Input/DefaultInput.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Component/Parkour/VaultComponent.h"
//#include "Component/Parkour/HangingComponent.h
#include "Component/Interaction/InteractionComponent.h"
#include "Component/Parkour/MantleComponent.h"
#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "UI/GameHUD.h"
#include "Weapons/Weaponactor.h"
#include "Weapons/WeaponPickUp.h"
#include "Weapons/WeaponInterface.h"


ACharacterPlayer::ACharacterPlayer(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

#pragma region MESH_SETTING

	USkeletalMeshComponent* MeshComp = GetMesh();
	ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple'"));

	if (MeshAsset.Succeeded())
	{
		MeshComp->SetSkeletalMesh(MeshAsset.Object);
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		MeshComp->SetRelativeRotation(FVector(0.f, -90.f, 0.f).Rotation());
	}

	ConstructorHelpers::FClassFinder<UAnimInstance> AnimAsset(TEXT("/Script/Engine.AnimBlueprint'/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C'"));
	if (AnimAsset.Succeeded())
	{
		MeshComp->SetAnimInstanceClass(AnimAsset.Class);
	}

#pragma endregion

	_SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SprintArm"));
	_CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));


	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

#pragma region ROTATION_SETTING

	if (IsValid(CapsuleComp))
	{
		_SpringArmComponent->SetupAttachment(CapsuleComp);
		//_CameraComponent->SetupAttachment(_SpringArmComponent);

		_SpringArmComponent->TargetArmLength = 0.f;
		_SpringArmComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));

		_SpringArmComponent->bUsePawnControlRotation = false;
		_SpringArmComponent->bInheritPitch = false;
		_SpringArmComponent->bInheritYaw = false;

		//_CameraComponent->bUsePawnControlRotation = false;
		_LookSensitivity = 0.75f;
	}

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (IsValid(MovementComp))
	{
		MovementComp->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
	}

#pragma endregion

#pragma region Mesh Visible Toggle // Arm SkeletalMesh 만 1인칭에게 그려주고 다른 사람은 Full Mesh 를 그리게 하기
	//// 팔의 위치와 회전은 카메라를 기준으로 조정합니다.
	//_FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	//_FirstPersonMesh->SetupAttachment(_CameraComponent);
	//// 자신의 화면에만 팔을 표시한다.
	//_FirstPersonMesh->SetOnlyOwnerSee(true);
	//// 화면 표현용 팔은 충돌과 그림자를 만들지 않는다.
	//_FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//_FirstPersonMesh->SetGenerateOverlapEvents(false);
	//_FirstPersonMesh->SetCastShadow(false);
	//// 기존 전신은 자신에게 숨기고 다른 플레이어에게 표시한다.
	//MeshComp->SetOwnerNoSee(true);

	_FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	_FirstPersonMesh->SetupAttachment(MeshComp);
	_CameraComponent->SetupAttachment(_FirstPersonMesh);
	_CameraComponent->bUsePawnControlRotation = true;
	_FirstPersonMesh->SetOnlyOwnerSee(true);
	_FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_FirstPersonMesh->SetGenerateOverlapEvents(false);
	_FirstPersonMesh->SetCastShadow(false);
	MeshComp->SetOwnerNoSee(true);

#pragma endregion
	
	// Parkour
	_HurdleCheckComponent = CreateDefaultSubobject<UHurdleCheckComponent>(TEXT("HurdleCheckComponent"));
	_VaultComponent = CreateDefaultSubobject<UVaultComponent>(TEXT("VaultComponent"));
	//_HangingComponent   = CreateDefaultSubobject<UHangingComponent>(TEXT("HangingComponent"));
	_MantleComponent = CreateDefaultSubobject<UMantleComponent>(TEXT("MantleComponent"));

	_InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

void ACharacterPlayer::SetAiming(bool bAniming)
{
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
	if (nullptr == _WeaponActorClass || WeaponID.IsNone())
	{
		return false;
	}
	
	if (false == IsValid(GetWorld()) || false == IsValid(_FirstPersonMesh) || false == _FirstPersonMesh->DoesSocketExist(TEXT("Shooter_Socket")))
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
	
	const bool bAttached = NewWeapon->AttachToComponent(_FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Shooter_Socket"));
	
	if (false == bAttached)
	{
		NewWeapon->Destroy();
		return false;
	}
	
	if (IsValid(_CurrentWeapon))
	{
		_CurrentWeapon->Destroy();
	}
	
	_CurrentWeapon = NewWeapon;
	
	OnWeaponEquiped();
	
	return true;
}

void ACharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	SetupPlayerMesh();
}

void ACharacterPlayer::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (nullptr != _AbilitySystemComponent)
	{
		_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
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
	UEnhancedInputLocalPlayerSubsystem* Subsystem = 
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (false == IsValid(Subsystem))
	{
		return;
	}

	Subsystem->AddMappingContext(_DefaultInput->_DefaultInputMappingContext.Get(), 0);

	InputComp->BindAction(_DefaultInput->_Move,       ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveAction);
	InputComp->BindAction(_DefaultInput->_Jump,       ETriggerEvent::Triggered, this, &ACharacterPlayer::Jump);
	InputComp->BindAction(_DefaultInput->_MouseLook,  ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveLookAction);
	InputComp->BindAction(_DefaultInput->_MouseZoom,  ETriggerEvent::Triggered, this, &ACharacterPlayer::CharacterMouseZoomAction);
	InputComp->BindAction(_DefaultInput->_Parkour,    ETriggerEvent::Started,   this, &ACharacterPlayer::ParkourAction);
	InputComp->BindAction(_DefaultInput->_Inventory,  ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleInventoryAction);
	InputComp->BindAction(_DefaultInput->_Map,		 ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleMapAction);
	InputComp->BindAction(_DefaultInput->_Interact,   ETriggerEvent::Started,   this, &ACharacterPlayer::InteractAction);
	InputComp->BindAction(_DefaultInput->_Fire,       ETriggerEvent::Started,   this, &ACharacterPlayer::FireAction);
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

void ACharacterPlayer::MoveAction(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();

	const FRotator Rotation		= Controller->GetControlRotation();
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

void ACharacterPlayer::CharacterMouseZoomAction(const FInputActionValue& Value)
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
	// 이전 코드인데 제가 우선은 WeaponPickUp한다고 수정하느라 기존코드는 전부 주석걸고 예외처리식으로 빼놨어요. 
	// 나중에 필요하면 WeaponPickUp 이랑 ItemPickUp 합칠때 주석 풀고 수정하면 될것같아요 - 건영 
	// Todo : MergeCode
	// if (_InteractionComponent)
	// 	_InteractionComponent->PickUpInteract();
	
	if (false == IsValid(_InteractionComponent))
	{
		return;
	}
	
	// WeaponPickUp 범위 안에서는 해당 무기를 우선 상호작용한다.
	if (IsValid(_NearbyWeaponPickUp))
	{
		_InteractionComponent->ServerInteract(_NearbyWeaponPickUp);
		
		return;
	}
	
	// 범위 내에 무기가 없으면 기존 작성되었던 Ray형식의 상호작용 방식을 사용하기
	_InteractionComponent->PickUpInteract();
}

void ACharacterPlayer::FireAction(const FInputActionValue& value)
{
	ServerFire();
}

void ACharacterPlayer::ServerFire_Implementation()
{
	if (false == IsValid(_CurrentWeapon))
	{
		return;
	}
	
	const FVector AimPoint = GetAimPoint(_CurrentWeapon->GetWeaponRange());
	
	_CurrentWeapon->Fire(AimPoint);
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

	MeshComponent->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);

	// TODO
	// 플레이어 몸통은 마테리얼로 나누는 걸 추천.
}
