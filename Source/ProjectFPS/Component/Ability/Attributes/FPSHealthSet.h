// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Component/Ability/Attributes/FPSAttributeSet.h"
#include "FPSHealthSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)	\
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)	\
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)	\
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)	\
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOutOfHealthDelegate);

/**
 * 
 */

struct FGameplayEffectModCallbackData;
UCLASS()
class PROJECTFPS_API UFPSHealthSet : public UFPSAttributeSet
{
	GENERATED_BODY()
	
public:
	UFPSHealthSet();

	ATTRIBUTE_ACCESSORS(UFPSHealthSet, _Health);
	ATTRIBUTE_ACCESSORS(UFPSHealthSet, _MaxHealth);
	ATTRIBUTE_ACCESSORS(UFPSHealthSet, _Shield);
	ATTRIBUTE_ACCESSORS(UFPSHealthSet, _MaxShield);
	ATTRIBUTE_ACCESSORS(UFPSHealthSet, _DamageIn);

	mutable FOutOfHealthDelegate OnOutOfHealth;
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_Health, meta = (ArrayClamp = true))
	FGameplayAttributeData _Health;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxHealth, meta = (ArrayClamp = true))
	FGameplayAttributeData _MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_Shield, meta = (ArrayClamp = true))
	FGameplayAttributeData _Shield;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxShield, meta = (ArrayClamp = true))
	FGameplayAttributeData _MaxShield;

	/**
	 * 체력을 깎기 전에 거쳐가는 최종 검사대.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", meta = (ArrayClamp = true))
	FGameplayAttributeData _DamageIn;
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 속성 값이 변경되기 직전에 호출 (Clamping 역할)
	 * 호출 시점: 어트리뷰트의 최대치(Base Value)가 변경되기 직전
	 * GE 뿐만 아니라 Setter 함수 직접 호출 등 모든 변경에 대응한다.
	 * 주로 값의 상한선 / 하한선을 제한하는데 쓰임.
	 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/**
	 * 호출 시점 : 게임플레이 이펙트(GE)로 인해 현재치가 정산되기 직전
	 * 오직 Gameplay Effect(GE)에 의해 변경된다.
	 * 데미지 계산, 무적 상태 체크, 실드 흡수 등에 적용된다.
	 * OutExecutionOutput을 통해 변경을 제안한다.
	 */
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;

	/**
	 * GameplayEffect(예: 데미지 GE)가 실행을 마친 후 호출
	 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxShield(const FGameplayAttributeData& OldValue);
};
