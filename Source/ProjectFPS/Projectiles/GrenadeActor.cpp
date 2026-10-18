// Fill out your copyright notice in the Description page of Project Settings.
#include "Projectiles/GrenadeActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Component/Ability/DamageSourceComponent.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Component/Ability/GamePlayAbility/FPSGrenadeAbility.h"
#include "GameMode/PlayerStateBase.h"
#include "GameTag/FPSGameplayTag.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/CharacterPlayer.h"
#include "Table/TableSubsystem.h"
#include "Table/TableDatas.h"
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

float AGrenadeActor::GetFuseTime() const
{
	return _FuseTime;
}

bool AGrenadeActor::CanCook() const
{
	return _bCanCook;
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
	
	FVector LaunchDirection = Direction.GetSafeNormal();
	
	if (LaunchDirection.IsNearlyZero() || false == FMath::IsFinite(_ThrowAngle))
	{
		return false;
	}
	
	// 조준 방향에서 위쪽으로 보정하고, 수직을 넘어 뒤집히지 않게 제한
	LaunchDirection = FRotator(FMath::Clamp(LaunchDirection.Rotation().Pitch + _ThrowAngle, -89.0, 89.0), LaunchDirection.Rotation().Yaw, 0.0).Vector();
	
	// 손을 떠날 때 현재 월드 위치와 회전을 유지한다.
	GetRootComponent()->UpdateComponentToWorld();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	_Thrown = true;
	
	OnRep_Thrown();
	
	// 투척 영역과 초기 속도 적용은 서버에서만 수정한다.
	_GrenadeCollision->SetEnableGravity(true);
	_GrenadeCollision->SetSimulatePhysics(true);
	_GrenadeCollision->SetPhysicsLinearVelocity(LaunchDirection * _ThrowSpeed);
	
	// 이미 시작된 타이머가 있으면 남은 시간을 유지
	if (GetLifeSpan() <= 0.f)
	{
		SetLifeSpan(_FuseTime); // 5초뒤에 수명이 끝남 -> 5초뒤에 터진다는 뜻
	}
	
	ForceNetUpdate();
	
	return true;
}

bool AGrenadeActor::Drop()
{
	if (!HasAuthority() || _Thrown || !IsValid(_GrenadeCollision) || GetLifeSpan() <= 0.f)
	{
		return false;
	}
	
	// 현재 손의 월드 위치에서 분리
	GetRootComponent()->UpdateComponentToWorld();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	// 기존 물리 이동 상태와 복제 처리를 사용
	_Thrown = true;
	
	OnRep_Thrown();
	
	_GrenadeCollision->SetEnableGravity(true);
	_GrenadeCollision->SetSimulatePhysics(true);
	
	// 투척 속도를 주지 않고 중력으로 떨어뜨리기
	_GrenadeCollision->SetPhysicsLinearVelocity(FVector::ZeroVector);
	
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
	
	if (HasAuthority())
	{
		// 투척자가 먼저 죽어도 처치 기록에 사용할 PlayerState를 보관
		UDamageSourceComponent* Source = NewObject<UDamageSourceComponent>(this);
		
		AddInstanceComponent(Source);
		
		const APawn* InstigatorPawn = GetInstigator();
		
		Source->SetSourcePlayerState(IsValid(InstigatorPawn) ? InstigatorPawn->GetPlayerState<APlayerStateBase>() : nullptr);
		
		Source->RegisterComponent();
	}

	// 같은 메시를 쓰는 투척물은 아이템 테이블의 색으로 구분 (각 화면에서 적용)
	if (UTableSubsystem* Table = UTableSubsystem::Get(this))
	{
		if (const FItemData* ItemData = Table->FindTableRow<FItemData>(TEXT("ItemTable"), _TID))
		{
			ItemData->ApplyTint(_GrenadeMesh);
		}
	}
}

void AGrenadeActor::MulticastPlayExplosion_Implementation(const FVector& Location)
{
	// 리슨서버는 해당되지않음, 전용서버를 가지게 되면 그때 적용되는 코드 -> 리슨처럼 직접 플레이하지 않는 서버에서는 그리지말라는 코드에요
	if (GetNetMode() == NM_DedicatedServer) 
	{
		return;
	}
	
	// 폭발 위치에 Niagara 이펙트를 생성하고 즉시 재생
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, _ExplosionEffect.Get(), Location, FRotator::ZeroRotator, FVector(1.f), true, true);
	

	// 폭발음도 같은 위치에서 재생한다. 수류탄 액터가 사라져도 소리는 끝까지 재생함.
	if (IsValid(_ExplosionSound.Get()))
	{
		UGameplayStatics::PlaySoundAtLocation(this, _ExplosionSound.Get(), Location);
	}
	
}

void AGrenadeActor::MulticastPlayCookSound_Implementation(const FVector& Location)
{
	// 플레이 화면과 소리가 없는 전용 서버에서는 재생하지 않는다.
	if (GetNetMode() == NM_DedicatedServer || !IsValid(_CookSound.Get()))
	{
		return;
	}
	
	UGameplayStatics::PlaySoundAtLocation(this, _CookSound.Get(), Location);
}

void AGrenadeActor::LifeSpanExpired()
{
	if (!HasAuthority() || !IsValid(GetWorld()))
	{
		Super::LifeSpanExpired();
		
		return;
	}
	
	// 현재 수류탄 위치를 폭발 연출과 피해 계산에 같이 사용
	const FVector ExplosionLocation = GetActorLocation(); // 현재 수류탄 위치 사용
	
	MulticastPlayExplosion(ExplosionLocation);			  // 해당 위치에서 폭발 이펙트를 재생하도록 알림
	
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetOwner());
	
	// 이 수류탄을 아직 손에 들고 있는 경우에만 장착과 동착을 정리
	if (IsValid(Character) && Character->GetEquippedGrenade() == this)
	{
		Character->ClearGrenadeReference(this);
		
		UFPSAbilitySystemComponent* AbilitySystem = Cast<UFPSAbilitySystemComponent>(Character->GetAbilitySystemComponent());
		
		if (IsValid(AbilitySystem) && AbilitySystem->GrenadeAbilityClass != nullptr)
		{
			if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(AbilitySystem->GrenadeAbilityClass))
			{
				if (Spec->IsActive())
				{
					AbilitySystem->CancelAbilityHandle(Spec->Handle);
				}
			}
		}
	}
	else
	{
		Character = nullptr;
	}
	
	// 종류마다 다른 폭발 처리 (기본 = 범위 데미지)
	Detonate(ExplosionLocation);

	// 손에서 폭발한 뒤 살아 있다면 기존 경로로 다음 수류탄을 장착
	if (IsValid(Character))
	{
		const UFPSAbilitySystemComponent* AbilitySystem = Cast<UFPSAbilitySystemComponent>(Character->GetAbilitySystemComponent());
		
		if (IsValid(AbilitySystem) && AbilitySystem->CanAttack() && !IsValid(Character->GetEquippedGrenade()))
		{
			Character->ServerEquipGrenade();
		}
	}
	
	// 피해처리가 끝난 수류탄을 제거
	// 발동 과정에서 새 수명을 설정한 투척물은 유지
	// 일반 수류탄처럼 추가 수명이 없는 경우에는 바로 제거
	if (GetLifeSpan() <= 0.f)
	{
		Super::LifeSpanExpired();	
	}
}

void AGrenadeActor::Detonate(const FVector& ExplosionLocation)
{
	// 종류별 자식 클래스가 구현한다. (폭탄 = 범위 데미지, 섬광탄 = 섬광, 화염병 = 불 장판)
}

bool AGrenadeActor::IsThrown() const
{
	return _Thrown;
}

void AGrenadeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
