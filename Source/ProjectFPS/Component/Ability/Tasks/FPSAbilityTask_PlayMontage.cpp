#include "Component/Ability/Tasks/FPSAbilityTask_PlayMontage.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
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

	if (!Ability || !IsValid(Character) || !Character->HasAuthority() || Character->IsDeathStarted())
	{
		EndTask();
		return;
	}

	_Character = Character;

	_AbilityCancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(
		this, &UFPSAbilityTask_PlayMontage::OnAbilityCancelled);

	// ServerOnly Ability도 소유 플레이어의 팔에서 재생할 수 있도록 Client RPC를 사용한다.
	if (IsValid(_FistPersonMontage) && FMath::IsFinite(_FPPlayRate) && _FPPlayRate > 0.f)
	{
		_FirstPersonPlayRequested = true;
		Character->ClientPlayMontage(EAnimMeshType::FirstPerson, _FistPersonMontage, _FPPlayRate);
	}

	if (IsValid(_ThirdPersonMontage) && FMath::IsFinite(_TPPlayRate) && _TPPlayRate > 0.f)
	{
		if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
		{
			// Client RPC만으로는 다른 플레이어에게 보이지 않는다. 서버 몸에서 재생하고
			// GAS가 몽타주/속도/위치/정지 상태를 simulated proxy에 복제하게 한다.
			_ThirdPersonMontageStarted = ASC->PlayMontage(
				Ability, Ability->GetCurrentActivationInfo(), _ThirdPersonMontage, _TPPlayRate) > 0.f;
		}

		// Montage_Play의 콜백에서 Ability가 종료됐으면 새 표시 요청을 보내지 않는다.
		if (!Ability->IsActive())
		{
			if (_ThirdPersonMontageStarted)
			{
				if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
				{
					if (ASC->GetAnimatingAbility() == Ability && ASC->GetCurrentMontage() == _ThirdPersonMontage)
					{
						ASC->CurrentMontageStop(_ThirdPersonMontage->GetDefaultBlendOutTime());
					}
				}
				_ThirdPersonMontageStarted = false;
			}
			return;
		}

		if (!_ThirdPersonMontageStarted)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to play third-person montage %s on %s. Check the body AnimInstance and skeleton."),
				*GetNameSafe(_ThirdPersonMontage), *GetNameSafe(Character));
		}
		else if (!Character->IsLocallyControlled())
		{
			// GAS는 소유 클라이언트의 몽타주를 재생하지 않으므로 별도 표시한다.
			// 호스트는 서버 몸에서 이미 재생 중이어서 RPC로 다시 시작하지 않는다.
			_ThirdPersonPlayRequested = true;
			Character->ClientPlayMontage(EAnimMeshType::ThirdPerson, _ThirdPersonMontage, _TPPlayRate);
		}
	}

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

	if (_StopWhenAbilityEnds || !bAbilityEnded)
	{
		if (_ThirdPersonMontageStarted)
		{
			if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
			{
				// 뒤이어 시작된 다른 Ability의 몽타주는 정지시키지 않는다.
				if (ASC->GetAnimatingAbility() == Ability && ASC->GetCurrentMontage() == _ThirdPersonMontage)
				{
					ASC->CurrentMontageStop(_ThirdPersonMontage->GetDefaultBlendOutTime());
				}
			}
		}

		if (ACharacterPlayer* Character = _Character.Get())
		{
			if (_FirstPersonPlayRequested && IsValid(_FistPersonMontage))
			{
				Character->ClientStopMontage(EAnimMeshType::FirstPerson, _FistPersonMontage);
			}

			if (_ThirdPersonPlayRequested && IsValid(_ThirdPersonMontage))
			{
				Character->ClientStopMontage(EAnimMeshType::ThirdPerson, _ThirdPersonMontage);
			}

		}
	}

	_FirstPersonPlayRequested = false;
	_ThirdPersonPlayRequested = false;
	_ThirdPersonMontageStarted = false;
	_Character.Reset();
	Super::OnDestroy(bAbilityEnded);
}
