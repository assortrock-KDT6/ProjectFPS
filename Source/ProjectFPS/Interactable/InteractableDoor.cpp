// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactable/InteractableDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AInteractableDoor::AInteractableDoor()
{
	// 움직이는 동안에만 Tick을 켠다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	_Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(_Root);

	// 문짝 = 경첩(Scene) 아래 메시. 움직임은 경첩에 적용한다.
	_LeftHinge = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHinge"));
	_LeftHinge->SetupAttachment(_Root);

	_LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
	_LeftDoorMesh->SetupAttachment(_LeftHinge);

	_RightHinge = CreateDefaultSubobject<USceneComponent>(TEXT("RightHinge"));
	_RightHinge->SetupAttachment(_Root);

	_RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
	_RightDoorMesh->SetupAttachment(_RightHinge);

	// 기본값: 한 짝 여닫이 (오른쪽 문짝은 메시를 지정하면 사용)
	_LeftMotion._OpenRotation = FRotator(0.f, 90.f, 0.f);
	_RightMotion._OpenRotation = FRotator(0.f, -90.f, 0.f);
}

void AInteractableDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractableDoor, _bOpen);
}

void AInteractableDoor::BeginPlay()
{
	Super::BeginPlay();

	_LeftClosedTransform = _LeftHinge->GetRelativeTransform();
	_RightClosedTransform = _RightHinge->GetRelativeTransform();
}

void AInteractableDoor::Interact_Implementation(AActor* Interactor)
{
	// ServerInteract에서 거리·인터페이스 검증을 마친 뒤 서버에서만 들어온다.
	if (false == HasAuthority())
	{
		return;
	}

	SetOpen(false == _bOpen);
}

void AInteractableDoor::SetOpen(bool bOpen)
{
	if (bOpen == _bOpen)
	{
		return;
	}

	_bOpen = bOpen;

	// 자동 닫힘: 열렸을 때만 예약하고, 닫혔으면 예약을 취소한다.
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(_AutoCloseTimer);
	if (true == _bOpen && _AutoCloseDelay > 0.f)
	{
		TimerManager.SetTimer(_AutoCloseTimer, this, &AInteractableDoor::CloseByTimer, _AutoCloseDelay, false);
	}

	// 리슨 서버의 호스트 화면은 OnRep이 불리지 않으므로 직접 처리한다.
	HandleOpenChanged();
}

void AInteractableDoor::CloseByTimer()
{
	SetOpen(false);
}

void AInteractableDoor::OnRep_Open()
{
	HandleOpenChanged();
}

void AInteractableDoor::HandleOpenChanged()
{
	USoundBase* Sound = nullptr;
	if (true == _bOpen)
	{
		Sound = _OpenSound;
	}
	else
	{
		Sound = _CloseSound;
	}

	if (nullptr != Sound && NM_DedicatedServer != GetNetMode())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, _LeftDoorMesh->GetComponentLocation());
	}

	SetActorTickEnabled(true);
}

void AInteractableDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float Target = 0.f;
	if (true == _bOpen)
	{
		Target = 1.f;
	}

	_OpenAlpha = FMath::FInterpConstantTo(_OpenAlpha, Target, DeltaSeconds, 1.f / _MoveDuration);
	ApplyDoorTransform();

	// 끝까지 움직였으면 Tick을 끈다.
	if (true == FMath::IsNearlyEqual(_OpenAlpha, Target))
	{
		SetActorTickEnabled(false);
	}
}

void AInteractableDoor::ApplyDoorTransform()
{
	// 끝에서 부드럽게 멈추도록 보간 곡선을 준다.
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, _OpenAlpha, 2.f);

	ApplyLeafTransform(_LeftHinge, _LeftClosedTransform, _LeftMotion, Eased);
	ApplyLeafTransform(_RightHinge, _RightClosedTransform, _RightMotion, Eased);
}

void AInteractableDoor::ApplyLeafTransform(USceneComponent* Hinge, const FTransform& ClosedTransform, const FDoorLeafMotion& Motion, float Alpha)
{
	if (nullptr == Hinge)
	{
		return;
	}

	// 경첩은 자기 위치를 축으로 돌고(여닫이), 이동량만큼 미끄러진다(미닫이·미서기).
	const FVector Location = ClosedTransform.GetLocation() + Motion._OpenOffset * Alpha;
	const FQuat Rotation = (Motion._OpenRotation * Alpha).Quaternion() * ClosedTransform.GetRotation();
	Hinge->SetRelativeLocationAndRotation(Location, Rotation);
}
