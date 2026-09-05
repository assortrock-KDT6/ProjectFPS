// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "FPSAbilitySystemComponent.generated.h"

struct FGameplayTag;
struct FGameplayTagContainer;

/**
 * 모든 전투 기능을 처리하는 거대한 컴포넌트를 만들면 안된다.
 * 
 * 해당 어빌리티 컴포넌트는 Actor 당 하나만 가질 수 있다.
 * 
 * 개별 사격 로직 -> GA_Fire
 * 재장전 로직 -> GA_Reload
 * 피해 공식 -> GameplayEffectExecutionCalculation
 * 체력 제한과 사망 판정 -> AttributeSet
 * 탄창과 무기 상태 -> Weapon / Equipment Component
 * 실제 이동 처리 -> Character Movement Component
 * 파티클.사운드 -> Gameplay Cue
 * 
 * 캐릭터가 가진 능리겨, 상태, 태그를 관리한다.
 */
UCLASS()
class PROJECTFPS_API UFPSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
//public:
//	void AbilityInputTagPressed(const FGameplayTag& InputTag);
//	void AbilityInputTagReleased(const FGameplayTag& InputTag);
//	void ProcessAbilityInput(float DeltaTime);
//	void CancelAbilitiesByTag(const FGameplayTagContainer& AbilityTags);
//	void CancelAbilitiesOnDeath();
};
