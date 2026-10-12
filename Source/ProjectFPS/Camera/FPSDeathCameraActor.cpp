#include "Camera/FPSDeathCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Character/CharacterPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

AFPSDeathCameraActor::AFPSDeathCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	_CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("DeathCamera"));
	SetRootComponent(_CameraComponent);
	_CameraComponent->FieldOfView = 70.f;
}

void AFPSDeathCameraActor::InitializeDeathCamera(ACharacterPlayer* DeadCharacter, double DeathEndServerTime,
	const FVector& FallbackTargetLocation, const FRotator& CameraRotation)
{
	_DeadCharacter = DeadCharacter;
	_LastTargetLocation = FallbackTargetLocation;
	_RetreatDirection = -FRotator(0.f, CameraRotation.Yaw, 0.f).Vector();
	_StartServerTime = GetServerTime();
	_EndServerTime = FMath::IsFinite(DeathEndServerTime)
		? FMath::Max(_StartServerTime + 0.01, DeathEndServerTime) : _StartServerTime + 0.01;
	_Initialized = true;

	UpdateCamera();
}

void AFPSDeathCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (true == _Initialized)
	{
		UpdateCamera();
	}
}

double AFPSDeathCameraActor::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
	return IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : (IsValid(World) ? World->GetTimeSeconds() : 0.0);
}

void AFPSDeathCameraActor::UpdateCamera()
{
	ACharacterPlayer* DeadCharacter = _DeadCharacter.Get();
	if (true == IsValid(DeadCharacter))
	{
		const USkeletalMeshComponent* Mesh = DeadCharacter->GetMesh();
		_LastTargetLocation = IsValid(Mesh) && Mesh->DoesSocketExist(_FocusSocket)
			? Mesh->GetSocketLocation(_FocusSocket) : DeadCharacter->GetActorLocation();
		_LastTargetLocation.Z += _FocusHeight;
	}

	// 서버의 리스폰 시각까지 후퇴한 뒤 새 Pawn이 준비될 때까지 마지막 구도를 유지한다.
	const float Progress = static_cast<float>(FMath::Clamp(
		(GetServerTime() - _StartServerTime) / (_EndServerTime - _StartServerTime), 0.0, 1.0));
	const float Distance = FMath::Max(0.f, _Distance) + FMath::Max(0.f, _AdditionalRetreatDistance) * Progress;
	FVector CameraLocation = _LastTargetLocation + _RetreatDirection * Distance + FVector(0.f, 0.f, _Height);

	if (true == IsValid(GetWorld()))
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DeathCamera), false, this);
		if (true == IsValid(DeadCharacter))
		{
			QueryParams.AddIgnoredActor(DeadCharacter);
		}
		const FVector DesiredLocation = CameraLocation;
		FVector TraceStart = _LastTargetLocation;
		for (int32 Attempt = 0; Attempt < 3; ++Attempt)
		{
			FHitResult Hit;
			if (false == GetWorld()->SweepSingleByChannel(Hit, TraceStart, DesiredLocation, FQuat::Identity,
				ECC_Camera, FCollisionShape::MakeSphere(FMath::Max(1.f, _CollisionProbeRadius)), QueryParams))
			{
				CameraLocation = DesiredLocation;
				break;
			}
			if (false == Hit.bStartPenetrating)
			{
				CameraLocation = Hit.Location;
				break;
			}

			// 시체가 벽/바닥 가까이에 있어 검사 시작 구가 겹친 경우 먼저 겹침을 해소한다.
			TraceStart += Hit.Normal * (FMath::Max(0.f, Hit.PenetrationDepth) + 2.f);
			CameraLocation = TraceStart;
		}
	}

	const FVector LookDirection = _LastTargetLocation - CameraLocation;
	const FRotator CameraRotation = LookDirection.IsNearlyZero() ? GetActorRotation() : LookDirection.Rotation();
	SetActorLocationAndRotation(CameraLocation, CameraRotation);
}
