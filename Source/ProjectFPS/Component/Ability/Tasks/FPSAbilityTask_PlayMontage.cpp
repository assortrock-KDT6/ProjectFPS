#include "Component/Ability/Tasks/FPSAbilityTask_PlayMontage.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Character/CharacterPlayer.h"
#include "Common/GameDefines.h"

UFPSAbilityTask_PlayMontage* UFPSAbilityTask_PlayMontage::PlayMontage(
	UGameplayAbility* OwningAbility, UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float FPPlayRate , float TPPlayRate ,
	bool bStopWhenAbilityEnds)
{
	UFPSAbilityTask_PlayMontage* Task = NewAbilityTask<UFPSAbilityTask_PlayMontage>(OwningAbility);
	Task->_FistPersonMontage = FirstPersonMontage;
	Task->_ThirdPersonMontage = ThirdPersonMontage;
	Task->_FPPlayRate = FPPlayRate;
	Task->_TPPlayRate = TPPlayRate;
	Task->_StopWhenAbilityEnds = bStopWhenAbilityEnds;
	return Task;
}

void UFPSAbilityTask_PlayMontage::Activate()
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActor());

	if (!Ability || !IsValid(Character) || !IsValid(_FistPersonMontage) || !IsValid(_ThirdPersonMontage) ||
		!FMath::IsFinite(_FPPlayRate) || _FPPlayRate <= 0.f || !FMath::IsFinite(_TPPlayRate) || _TPPlayRate <= 0.f ||
		(!Character->HasAuthority() && !Character->IsLocallyControlled()))
	{
		EndTask();
		return;
	}

	_Character = Character;

	_PlayRequested = true;

	_AbilityCancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(
		this, &UFPSAbilityTask_PlayMontage::OnAbilityCancelled);

	// ServerOnly Ability도 소유 플레이어의 팔에서 재생할 수 있도록 Client RPC를 사용한다.
	Character->ClientPlayMontage(EAnimMeshType::FirstPerson, _FistPersonMontage, _FPPlayRate);
	Character->ClientPlayMontage(EAnimMeshType::ThirdPerson, _ThirdPersonMontage, _TPPlayRate);

	// 재생 완료는 기다리지 않고, Ability 종료까지 남아 정지를 담당한다.
}

void UFPSAbilityTask_PlayMontage::OnAbilityCancelled()
{
	EndTask();
}

void UFPSAbilityTask_PlayMontage::OnDestroy(bool bAbilityEnded)
{
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(_AbilityCancelledHandle);
	}

	if (_PlayRequested && (_StopWhenAbilityEnds || !bAbilityEnded))
	{
		if (ACharacterPlayer* Character = _Character.Get())
		{
			if (true == IsValid(_FistPersonMontage))
			{
				Character->ClientStopMontage(EAnimMeshType::FirstPerson, _FistPersonMontage);
			}

			if (true == IsValid(_ThirdPersonMontage))
			{
				Character->ClientStopMontage(EAnimMeshType::ThirdPerson, _ThirdPersonMontage);
			}

		}
	}

	_PlayRequested = false;
	_Character.Reset();
	Super::OnDestroy(bAbilityEnded);
}
