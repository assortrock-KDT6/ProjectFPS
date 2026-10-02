// Fill out your copyright notice in the Description page of Project Settings.
#include "Projectiles/GrenadeActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AGrenadeActor::AGrenadeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 서버가 생성할 실물과 이동 상태를 클라이언트에 전달한다.
	bReplicates = true;
	
	SetReplicateMovement(true);
	
	// Collision
	_GrenadeCollision = CreateDefaultSubobject<USphereComponent>(TEXT("GrenadeCollision"));
	SetRootComponent(_GrenadeCollision);
	_GrenadeCollision -> SetCollisionEnabled(ECollisionEnabled::NoCollision); // 손에 들고 있는 동아넹는 충동하지 않는다.
	_GrenadeCollision -> SetGenerateOverlapEvents(false);
	
	// Mesh
	_GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	_GrenadeMesh -> SetupAttachment(_GrenadeCollision);
	_GrenadeMesh -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_GrenadeMesh -> SetGenerateOverlapEvents(false);
	
	// 장착 중 소유자는 별도의 1인칭 메시를 보고, 다른 플레이어는 이 실물 메시를 본다.
	_GrenadeMesh -> SetOwnerNoSee(true);
}

UStaticMeshComponent* AGrenadeActor::GetGrenadeMesh() const
{
	return _GrenadeMesh;
}

FName AGrenadeActor::GetTID() const
{
	return _TID;
}

bool AGrenadeActor::Throw(const FVector& Direction)
{
	if (false == HasAuthority() ||
		_Thrown ||
		false == IsValid(_GrenadeCollision) ||
		Direction.ContainsNaN() ||
		false == FMath::IsFinite(_ThrowSpeed) ||
		_ThrowSpeed <= 0.f)
	{
		return false;
	}
	
	const FVector LaunchDirection = Direction.GetSafeNormal();
	
	if (LaunchDirection.IsNearlyZero())
	{
		return false;
	}
	
	// 손을 떠날 때 현재 월드 위치와 회전을 유지한다.
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	_Thrown = true;
	
	OnRep_Thrown();
	
	// 투척 영역과 초기 속도 적용은 서버에서만 수정한다.
	_GrenadeCollision->SetEnableGravity(true);
	_GrenadeCollision->SetSimulatePhysics(true);
	_GrenadeCollision->SetPhysicsLinearVelocity(LaunchDirection * _ThrowSpeed);
	
	ForceNetUpdate();
	
	return true;
}

void AGrenadeActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrenadeActor, _Thrown);
}

void AGrenadeActor::OnRep_Thrown()
{
	if (false == _Thrown)
	{
		return;
	}
	
	// 던져진 실물은 소유 플레이어에게로 보여야 한다.
	_GrenadeMesh->SetOwnerNoSee(false);
	_GrenadeCollision->SetCollisionProfileName(TEXT("PhysicsActor"));
	
	// 캐릭터 캡슐과의 접촉으로 투척직후 밀려나는걸 방지
	_GrenadeCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	
	// 빠르게 이동할 때 얇은 벽을 통과하는 현상을 줄이기
	_GrenadeCollision->SetUseCCD(true);
}

void AGrenadeActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGrenadeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
