#include "Character/CharacterPlayer.h"
#include "Controller/PlayerControllerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Input/DefaultInput.h"
#include "Component/Parkour/HurdleCheckComponent.h"
#include "Component/Parkour/ParkourComponent.h"
//#include "Component/Parkour/HangingComponent.h
#include "Component/Parkour/MantleComponent.h"
#include "UI/GameHUD.h"


ACharacterPlayer::ACharacterPlayer()
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
	_CameraComponent	= CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

#pragma region ROTATION_SETTING

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (IsValid(CapsuleComp))
	{
		_SpringArmComponent->SetupAttachment(CapsuleComp);
		_CameraComponent->SetupAttachment(_SpringArmComponent);

		_SpringArmComponent->TargetArmLength = 0.f;
		_SpringArmComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
		
		_SpringArmComponent->bUsePawnControlRotation = true;
		_SpringArmComponent->bInheritPitch	= true;
		_SpringArmComponent->bInheritYaw	= true;

		_CameraComponent->bUsePawnControlRotation = false;
		_LookSensitivity = 0.75f;
	}

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();	
	if (IsValid(MovementComp))
	{
		MovementComp->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
	}

#pragma endregion

	// Parkour
	_HurdleCheckComponent = CreateDefaultSubobject<UHurdleCheckComponent>(TEXT("HurdleCheckComponent"));
	_ParkourComponent     = CreateDefaultSubobject<UParkourComponent>(TEXT("ParkourComponent"));
	//_HangingComponent   = CreateDefaultSubobject<UHangingComponent>(TEXT("HangingComponent"));
	_MantleComponent    = CreateDefaultSubobject<UMantleComponent>(TEXT("MantleComponent"));

}

void ACharacterPlayer::BeginPlay()
{
	Super::BeginPlay();
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

	InputComp->BindAction(_DefaultInput->_Move,      ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveAction);
	InputComp->BindAction(_DefaultInput->_Jump,      ETriggerEvent::Triggered, this, &ACharacterPlayer::Jump);
	InputComp->BindAction(_DefaultInput->_MouseLook, ETriggerEvent::Triggered, this, &ACharacterPlayer::MoveLookAction);
	InputComp->BindAction(_DefaultInput->_MouseZoom, ETriggerEvent::Triggered, this, &ACharacterPlayer::CharacterMouseZoomAction);
	InputComp->BindAction(_DefaultInput->_Parkour,   ETriggerEvent::Started,   this, &ACharacterPlayer::ParkourAction);
	InputComp->BindAction(_DefaultInput->_Inventory, ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleInventoryAction);
	InputComp->BindAction(_DefaultInput->_Map,		ETriggerEvent::Started,   this, &ACharacterPlayer::ToggleMapAction);
}

void ACharacterPlayer::PossessedBy(AController* Newcontroller)
{
	Super::PossessedBy(Newcontroller);
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

	const bool bVaultActive = IsValid(_ParkourComponent) && _ParkourComponent->IsVaultActive();
	
	const bool bMantleActive = IsValid(_MantleComponent) && _MantleComponent->IsMantleActive();
	
	if (bVaultActive || bMantleActive)
	{
		return;
	}
	
	if (IsValid(_ParkourComponent) && _ParkourComponent->TryParkour())
	{
		return;
		
		// if (_ParkourComponent->TryParkour())
		// {
		// 	return;
		// }
	}
	if (IsValid(_MantleComponent))
	{
		_MantleComponent->TryMantle();
		// _ParkourComponent->TryParkour(); <-- Fatal Bug FIX : 두개의 파쿠르 액션을 취하게 되니 Flying 상태에서 다시 Flying 상태가 되어 이전 상태인 Move_Walk를 기억하지못해 계속된 Flying 상태가 유지됐다.
		//  todo : 애니메이션 몽타쥬가 종료될때까지 액션이 끝날동안 추가 파쿠르 입력의 키는 받지 않게 설정한다.
	}
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
