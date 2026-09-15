// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "FPSAnimInstance.generated.h"

/**
 * 
 */

class FDataValidationContext;
class UFPSCharacterMovementComponent;

UCLASS()
class PROJECTFPS_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UFPSAnimInstance(const FObjectInitializer& ObjectInitializer);

protected:
	/**
	 * 컨트롤 릭 
	 */

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Traversal IK")
	FTransform _LeftHandTarget;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Traversal IK")
	FTransform _RightHandTarget;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Traversal IK")
	FTransform _LeftFootTarget;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Traversal IK")
	FTransform _RightFootTarget;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Traversal IK")
	FTransform _PelvisTarget;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Traversal IK")
	float _TraversalIKAlpha = 0.f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Anim")
	TObjectPtr<ACharacter> _Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Anim")
	TObjectPtr<UFPSCharacterMovementComponent> _OwnerMovement = nullptr;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Anim|GameplayTags")
	FGameplayTagBlueprintPropertyMap _GameplayTagPropertyMap;

	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float _GroundDistance = -1.f;

protected:

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent);

#if WITH_EDITOR
	/**
	 * 태그 오류를 잡아주는 역할을 한다.
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif //WITH_EDITOR

	virtual void NativeInitializeAnimation() override;

	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

private:
	void UpdateTraversalIK();
};
