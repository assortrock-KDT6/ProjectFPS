// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace FPSGameplayTags
{
	FGameplayTag	FindTagByString(const FString& TagString, bool MatchPartialString = false);

	extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_IsDead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_Cost);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_Interval);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActiveteFail_TagsBlocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_TagsMissing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_Networking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_ActivationGroup);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Heal);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Crouching);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Walking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Falling);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Swimming);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Flying);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_NavWalking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Custom);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Vault);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Mantle);
}