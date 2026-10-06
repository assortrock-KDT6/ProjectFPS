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
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/CharacterPlayer.h"
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
	
	FVector LaunchDirection = Direction.GetSafeNormal();
	
	if (LaunchDirection.IsNearlyZero() || false == FMath::IsFinite(_ThrowAngle))
	{
		return false;
	}
	
	// 조준 방향에서 위쪽으로 보정하고, 수직을 넘어 뒤집히지 않게 제한
	LaunchDirection = FRotator(FMath::Clamp(LaunchDirection.Rotation().Pitch + _ThrowAngle, -89.0, 89.0), LaunchDirection.Rotation().Yaw, 0.0).Vector();
	
	// 손을 떠날 때 현재 월드 위치와 회전을 유지한다.
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
		SetLifeSpan(5.f); // 5초뒤에 수명이 끝남 -> 5초뒤에 터진다는 뜻
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
}

void AGrenadeActor::LifeSpanExpired()
{
	if (!HasAuthority() || !IsValid(GetWorld()))
	{
		Super::LifeSpanExpired();
		
		return;
	}
	
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
	
	if (_DamageEffect == nullptr || !FMath::IsFinite(_ExplosionRadius) || _ExplosionRadius <= 0.f || !FMath::IsFinite(_Damage) || _Damage <= 0.f)
	{
		Super::LifeSpanExpired();
		
		return;
	}
	
	const FVector ExplosionLocation = GetActorLocation();
	
	// 수류탄의 폭발범위에 걸친 Pawn을 검색 -> 이때 수류탄을 던진 플레이어도 포함
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 이코드는 잘 이해못함 무슨 문법인지 잘 모르겠어요 그래서 노션에 정리해 두었습니다. - 건영
	
	TArray<AActor*> ActorsToIgnore;
	
	ActorsToIgnore.Add(this);
	
	TArray<AActor*> Targets;
	
	UKismetSystemLibrary::SphereOverlapActors(this, ExplosionLocation, _ExplosionRadius, ObjectTypes, APawn::StaticClass(), ActorsToIgnore, Targets);
	
	const UDamageSourceComponent* Source = FindComponentByClass<UDamageSourceComponent>();
	
	APlayerStateBase* FPSPlayerState = IsValid(Source) ? Source->GetSourcePlayerState() : nullptr;
	
	// 플레이어끼리도 서로 폭발을 막는 벽으로 취급하지 않기
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GrenadeExplosion), false, this);
	
	TraceParams.AddIgnoredActors(Targets);
	
	for (AActor* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}
		
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		
		if (!IsValid(TargetASC))
		{
			continue;
		}
		
		// 폭발 지점부터 대상 중심까지 벽이 막고 있으면 피해를 주지 못함
		if (GetWorld()->LineTraceTestByChannel(ExplosionLocation, Target->GetActorLocation(), ECC_Visibility, TraceParams))
		{
			continue;
		}
		
		FGameplayEffectContextHandle Context(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());
		
		Context.AddInstigator(FPSPlayerState, this);
		Context.AddSourceObject(this);
		
		FGameplayEffectSpec Spec(_DamageEffect.GetDefaultObject(), Context, 1.f);
		
		Spec.SetSetByCallerMagnitude(FPSGameplayTags::SetByCaller_Damage, _Damage);
		
		TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	}

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
	Super::LifeSpanExpired();
}

void AGrenadeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
