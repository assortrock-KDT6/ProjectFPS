// Fill out your copyright notice in the Description page of Project Settings.
#include "FPSGameplayTag.h"

#include "Common/GameDefines.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagsManager.h"

namespace FPSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_IsDead,				"Ability.ActivateFail.IsDead",			"사용자가 죽었으므로 어빌리티를 사용할 수 없습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Cost,				"Ability.ActivateFail.Cost",			"사용자의 코스트가 부족하므로 사용할 수 없습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Interval,			"Ability.ActivateFail.Interval",		"Interval의 쿨타임이 진행되고 있습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActiveteFail_TagsBlocked,		"Ability.ActivateFail.TagsBlocked",		"어빌리티 사용이 막혀있습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_TagsMissing,		"Ability.ActivateFail.TagsMissing",		"해당 어빌리티를 찾을 수 없습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Networking,			"Ability.ActivateFail.Networking",		"네트워크 로딩 중에는 어빌리티를 사용할 수 없습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_ActivationGroup,	"Ability.ActivateFail.ActivationGroup", "해당 어빌리티는 이미 활동그룹에 들어가있습니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned,		"InitState.Spawned",		" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady, "InitState.GameplayReady",	" ");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,	"SetByCaller.Damage", " ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal,	"SetByCaller.Heal", " ");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Crouching,	"Status.Crouching",		" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_ADS, "Status.ADS", "Aiming down sights");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Firing, "Status.Firing", "Weapon firing animation window");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Reloading, "Status.Reloading", "Reload in progress");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Melee, "Status.Melee", "Melee in progress");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Dashing, "Status.Dashing", "Dash in progress");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death,		"Status.Death",			" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dying,	"Status.Death.Dying",	" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dead,	"Status.Death.Dead",	" ");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Walking,		"Movement.Mode.Walking",	" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_NavWalking,	"Movement.Mode.NavWalking",	" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Falling,		"Movement.Mode.Falling",	" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Swimming,		"Movement.Mode.Swimming",	" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Flying,		"Movement.Mode.Flying",		" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Custom,		"Movement.Mode.Custom",	" ");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Vault,		"Movement.Mode.Vault",	" ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Mantle,	"Movement.Mode.Mantle", " ");

	const TMap<uint8, FGameplayTag> MovementModeTagMap =
	{
		{ MOVE_Walking,		Movement_Mode_Walking },
		{ MOVE_NavWalking,	Movement_Mode_NavWalking },
		{ MOVE_Falling,		Movement_Mode_Falling },
		{ MOVE_Swimming,	Movement_Mode_Swimming },
		{ MOVE_Flying,		Movement_Mode_Flying },
		{ MOVE_Custom,		Movement_Mode_Custom}
	};

	const TMap<uint8, FGameplayTag> CustomMovementModeTagMap =
	{
		{ static_cast<uint8>(EProjectCustomMovementMode::Vault),	Movement_Mode_Vault },
		{ static_cast<uint8>(EProjectCustomMovementMode::Mantle),	Movement_Mode_Mantle }
	};

	FGameplayTag FPSGameplayTags::FindTagByString(const FString& TagString, bool MatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (false == Tag.IsValid() && true == MatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				Tag = TestTag;
				break;
			}
		}
		return Tag;
	}
}
