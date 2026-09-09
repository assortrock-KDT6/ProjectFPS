// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/FPSAnimInstance.h"
#include "AbilitySystemGlobals.h"
#include "Character/CharacterBase.h"
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

    _Owner = Cast<ACharacter>(GetOwningActor());

    if (nullptr != _Owner)
    {
        UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(_Owner);
        if (nullptr != AbilitySystemComponent)
        {
            InitializeWithAbilitySystem(AbilitySystemComponent);
        }
    }

    _OwnerMovement = Cast<UFPSCharacterMovementComponent>(_Owner->GetCharacterMovement());
    if (false == IsValid(_OwnerMovement))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("캐릭터 Movement Component 캐스트 실패"));
    }
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (nullptr == _Owner || nullptr == _OwnerMovement)
    {
        return;
    }

    const FCharacterGroundInfo& GroundInfomation = _OwnerMovement->GetGroundInfomation();
    _GroundDistance = GroundInfomation._GroundDistance;
}
