// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/FPSAnimInstance.h"
#include "AbilitySystemGlobals.h"
#include "GameTag/FPSGameplayTag.h"
#include "UObject/UnrealType.h"
#include "Character/CharacterPlayer.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif // WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(FPSAnimInstance)

UFPSAnimInstance::UFPSAnimInstance(const FObjectInitializer& ObjectInitializer)
    :Super(ObjectInitializer)
{
}

void UFPSAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent)
{
    check(AbilitySystemComponent);

    _GameplayTagPropertyMap.Initialize(this, AbilitySystemComponent);
}

void UFPSAnimInstance::NativeUninitializeAnimation()
{
    ResetTraversalIK();
    _Owner = nullptr;
    _OwnerMovement = nullptr;
    Super::NativeUninitializeAnimation();
}

#if WITH_EDITOR
EDataValidationResult UFPSAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
    Super::IsDataValid(Context);

    _GameplayTagPropertyMap.IsDataValid(this, Context);

    const EDataValidationResult Result = (Context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid;

    return Result;
}
#endif // WITH_EDITOR

void UFPSAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    ResetTraversalIK();

    _Owner = Cast<ACharacterPlayer>(GetOwningActor());
    
    if (nullptr == _Owner)
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(_Owner);
    if (nullptr != AbilitySystemComponent)
    {
        InitializeWithAbilitySystem(AbilitySystemComponent);
    }

    _OwnerMovement = Cast<UFPSCharacterMovementComponent>(_Owner->GetCharacterMovement());
    if (false == IsValid(_OwnerMovement))
    {
        if (nullptr != GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("캐릭터 Movement Component 캐스트 실패"));
        }
    }
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 병렬 애니메이션 평가가 시작되기 전에  게임플레이에서 관리하는 Traversal 데이터를 현재 프레임 기준으로 복사해 둔다.    

    UpdateTraversalIK(DeltaSeconds);

    _GroundIKEnabled = IsValid(_OwnerMovement) && _OwnerMovement->IsMovingOnGround() && false == _OwnerMovement->IsTraversing() && _TraversalIKAlpha <= KINDA_SMALL_NUMBER;

    // 이 호출은 월드에 Trace를 수행하며, Movement Component가 공유하는 프레임 단위 캐시 데이터를 변경한다.
    // 따라서 Body와 Arms 애니메이션에서는 애니메이션 Worker Thread에서 이 함수를 호출하면 안 된다.
    _GroundDistance = IsValid(_OwnerMovement) ? _OwnerMovement->GetGroundInfomation()._GroundDistance : -1.f;
}

void UFPSAnimInstance::ResetTraversalIK()
{
    _TraversalIKAlpha = 0.f;
    _TraversalContactAge = 0.f;
    _HasTraversalContacts = false;
    _GroundIKEnabled = false;
    _LeftRotationLocked = false;
    _RightRotationLocked = false;
}

void UFPSAnimInstance::UpdateTraversalIK(float DeltaSeconds)
{
    const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();

    if (false == IsValid(_OwnerMovement) || nullptr == Mesh)
    {
        ResetTraversalIK();
        return;
    }

    static const FName TraversalSlot(TEXT("FullBodyTraversal"));

    const float TraversalPoseWeight = FMath::Clamp(CalcSlotMontageLocalWeight(TraversalSlot), 0.f, 1.f);

    FTraversalContactTargets Contacts;

    if (true == _OwnerMovement->GetTraversalContactTargets(Contacts))
    {
        _CachedLeftHandWorld = Contacts._LeftHand;
        _CachedRightHandWorld = Contacts._RightHand;
        _CachedObstacleWorld = Contacts._ObstacleFrame;
        _TraversalObstacleDepth = Contacts._ObstacleDepth;
        _TraversalContactAge = 0.f;
        _HasTraversalContacts = true;

        // 장애물 보정 IK는 Traversal 포즈의 블렌딩 정도에 맞춰 함께 적용되고 함께 빠져야 한다.
        // Traversal 시작이나 종료 시점에 완전히 적용된 Locomotion 포즈를 IK가 갑자기 밀어내지 않도록 한다.
        _TraversalIKAlpha = TraversalPoseWeight;
    }
    else if (true == _HasTraversalContacts)
    {
        _TraversalContactAge += FMath::Max(0.f, DeltaSeconds);

        // Movement의 Traversal 상태가 먼저 종료되더라도 Montage 포즈는 아직 Blend Out 중일 수 있다.
        // 따라서 잠시 기존 Contact 정보를 유지하되, Montage가 중간에 끊긴 경우 오래된 IK 정보가 계속 남지 않도록 Traversal 포즈의 가중치를 기준으로 IK Alpha를 제한한다.
        _TraversalIKAlpha = FMath::Min(_TraversalIKAlpha, TraversalPoseWeight);
        if (_TraversalContactAge > 0.35f)
        {
            _TraversalIKAlpha = FMath::FInterpConstantTo(_TraversalIKAlpha, 0.f, DeltaSeconds, 1.f / 0.15f);
        }
        if (_TraversalIKAlpha <= KINDA_SMALL_NUMBER)
        {
            ResetTraversalIK();
        }
    }

    if (false == _HasTraversalContacts)
    {
        return;
    }

    UpdateContactRotation(TEXT("TraversalHandL"), TEXT("hand_l"), _LeftRotationLocked, _LeftContactRotation, _CachedLeftHandWorld);
    UpdateContactRotation(TEXT("TraversalHandR"), TEXT("hand_r"), _RightRotationLocked, _RightContactRotation, _CachedRightHandWorld);

    // 이 World Space Contact Target들은 캐릭터 이동이 완료된 이후, 애니메이션 평가 시점에 Control Rig에서 필요한 공간으로 변환한다.
    // 여기에서 미리 좌표계를 변환하면 Root Motion이 적용되기 전의 Component Transform을 사용할 가능성이 있다.

    _LeftHandTarget = _CachedLeftHandWorld;
    _RightHandTarget = _CachedRightHandWorld;
    _TraversalObstacleFrame = _CachedObstacleWorld;
}

void UFPSAnimInstance::UpdateContactRotation(const FName& Curve, const FName& Bone, bool& Locked, FQuat& Rotation, FTransform& Target)
{
    const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
    if (nullptr == Mesh)
    {
        // 예방 차원 return
        return;
    }
    const float Weight = GetCurveValue(Curve);
    if (Weight < 0.1f)
    {
        Locked = false;
    }
    if (false == Locked)
    {
        Rotation = Mesh->GetSocketQuaternion(Bone);
        Locked = Weight >= 0.5f;
    }
    Target.SetRotation(Rotation);
}
