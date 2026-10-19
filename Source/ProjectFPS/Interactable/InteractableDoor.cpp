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

	DOREPLIFETIME(AInteractableDoor, _OpenState);
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

	SetOpen(false == IsOpen(), Interactor);
}

void AInteractableDoor::SetOpen(bool bOpen, AActor* Interactor)
{
	if (bOpen == IsOpen())
	{
		return;
	}

	if (true == bOpen)
	{
		_OpenState = OpenFlag;
		if (IsValid(Interactor))
		{
			const FVector InteractorLocation = _Root->GetComponentTransform().InverseTransformPosition(Interactor->GetActorLocation());
			if (ShouldReverseLeaf(_LeftDoorMesh, _LeftClosedTransform, _LeftMotion, InteractorLocation))
			{
				_OpenState |= LeftReverseFlag;
			}
			if (ShouldReverseLeaf(_RightDoorMesh, _RightClosedTransform, _RightMotion, InteractorLocation))
			{
				_OpenState |= RightReverseFlag;
			}
		}
	}
	else
	{
		// 닫을 때는 선택했던 방향을 유지한다.
		_OpenState &= ~OpenFlag;
	}

	// 자동 닫힘: 열렸을 때만 예약하고, 닫혔으면 예약을 취소한다.
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(_AutoCloseTimer);
	if (true == IsOpen() && _AutoCloseDelay > 0.f)
	{
		TimerManager.SetTimer(_AutoCloseTimer, this, &AInteractableDoor::CloseByTimer, _AutoCloseDelay, false);
	}

	// 리슨 서버의 호스트 화면은 OnRep이 불리지 않으므로 직접 처리한다.
	HandleOpenChanged();
	ForceNetUpdate();
}

bool AInteractableDoor::ShouldReverseLeaf(const UStaticMeshComponent* DoorMesh, const FTransform& ClosedTransform,
	const FDoorLeafMotion& Motion, const FVector& InteractorLocation) const
{
	if (nullptr == DoorMesh || nullptr == DoorMesh->GetStaticMesh() || Motion._OpenRotation.IsNearlyZero())
	{
		return false;
	}

	// 메시 원점이 경첩에 있을 수도 있으므로 실제 로컬 바운드의 중심을 사용한다.
	FVector BoundsMin;
	FVector BoundsMax;
	DoorMesh->GetLocalBounds(BoundsMin, BoundsMax);
	const FVector CenterInHinge = DoorMesh->GetRelativeTransform().TransformPosition((BoundsMin + BoundsMax) * 0.5f);
	const FVector ClosedCenter = ClosedTransform.TransformPosition(CenterInHinge);
	const FVector HingeToCenter = ClosedCenter - ClosedTransform.GetLocation();

	// 작은 정/역회전의 차이는 닫힌 문이 처음 움직이는 방향(문 앞뒤의 법선)이다.
	// Root 로컬에서 계산하므로 액터 회전, 경첩 방향, 메시 배치에 의존하지 않는다.
	const FQuat ForwardStep = (Motion._OpenRotation * 0.01f).Quaternion();
	const FQuat ReverseStep = (Motion._OpenRotation * -0.01f).Quaternion();
	const FVector OpeningDirection = ForwardStep.RotateVector(HingeToCenter) - ReverseStep.RotateVector(HingeToCenter);
	return FVector::DotProduct(OpeningDirection, InteractorLocation - ClosedCenter) > 0.f;
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
	if (true == IsOpen())
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

	float LeftTarget = 0.f;
	float RightTarget = 0.f;
	if (true == IsOpen())
	{
		LeftTarget = (0 != (_OpenState & LeftReverseFlag)) ? -1.f : 1.f;
		RightTarget = (0 != (_OpenState & RightReverseFlag)) ? -1.f : 1.f;
	}

	const float Speed = 1.f / FMath::Max(_MoveDuration, 0.05f);
	_LeftOpenAlpha = FMath::FInterpConstantTo(_LeftOpenAlpha, LeftTarget, DeltaSeconds, Speed);
	_RightOpenAlpha = FMath::FInterpConstantTo(_RightOpenAlpha, RightTarget, DeltaSeconds, Speed);
	ApplyDoorTransform();

	// 끝까지 움직였으면 Tick을 끈다.
	if (FMath::IsNearlyEqual(_LeftOpenAlpha, LeftTarget) && FMath::IsNearlyEqual(_RightOpenAlpha, RightTarget))
	{
		SetActorTickEnabled(false);
	}
}

void AInteractableDoor::ApplyDoorTransform()
{
	ApplyLeafTransform(_LeftHinge, _LeftClosedTransform, _LeftMotion, _LeftOpenAlpha);
	ApplyLeafTransform(_RightHinge, _RightClosedTransform, _RightMotion, _RightOpenAlpha);
}

void AInteractableDoor::ApplyLeafTransform(USceneComponent* Hinge, const FTransform& ClosedTransform, const FDoorLeafMotion& Motion, float Alpha)
{
	if (nullptr == Hinge)
	{
		return;
	}

	// 경첩은 자기 위치를 축으로 돌고(여닫이), 이동량만큼 미끄러진다(미닫이·미서기).
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, FMath::Abs(Alpha), 2.f);
	const FVector Location = ClosedTransform.GetLocation() + Motion._OpenOffset * Eased;
	const FQuat Rotation = (Motion._OpenRotation * (FMath::Sign(Alpha) * Eased)).Quaternion() * ClosedTransform.GetRotation();
	Hinge->SetRelativeLocationAndRotation(Location, Rotation);
}
