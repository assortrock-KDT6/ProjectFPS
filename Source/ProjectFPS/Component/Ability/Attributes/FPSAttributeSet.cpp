// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Ability/Attributes/FPSAttributeSet.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"

UFPSAttributeSet::UFPSAttributeSet()
{
}

UWorld* UFPSAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UFPSAbilitySystemComponent* UFPSAttributeSet::GetFPSAttributeComponent() const
{
	return Cast<UFPSAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
