// 프로젝트 설정의 설명 페이지에 저작권 정보를 입력하세요.

#pragma once

#include "NativeGameplayTags.h"

namespace FPSGameplayTags
{
	FGameplayTag	FindTagByString(const FString& TagString, bool MatchPartialString = false);

	extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combat_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combat_Reload);
	// 근접 공격 능력과 몸 몽타주에서 전달하는 타격·콤보 이벤트이다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combat_Melee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_HitStart);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_HitEnd);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_ComboOpen);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_ComboClose);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Grenade_Cook);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Combat_Blocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Damage_Immune);

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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_ADS);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Firing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Sprinting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Reloading);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Melee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dashing);
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

	// 게임플레이 큐
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_Fire); // 총 발사음.

	// 총알 탄착 연출 (데칼)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_Impact);

	// 피격 연출(피 / 쉴드)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Character_Hit_Health);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Character_Hit_Shield);

}
