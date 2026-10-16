#include "Component/Ability/GamePlayAbility/FPSMeleeAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/CharacterPlayer.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Component/Ability/Attributes/FPSHealthSet.h"
#include "Component/Ability/GameEffect/FPSMeleeDamageEffect.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameTag/FPSGameplayTag.h"
#include "TimerManager.h"
#include "Weapons/WeaponActor.h"

UFPSMeleeAbility::UFPSMeleeAbility()
{
	// 소유 클라이언트는 즉시 동작을 예측하고 서버는 같은 능력 활성화를 검증한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(FPSGameplayTags::Ability_Combat_Melee);
	SetAssetTags(Tags);

	// 근접 공격 상태는 GAS가 활성화와 종료 시 부여하고 해제한다.
	ActivationOwnedTags.AddTag(FPSGameplayTags::Status_Melee);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Reloading);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Dashing);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Vault);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Mantle);
	BlockAbilitiesWithTag.AddTag(FPSGameplayTags::Ability_Combat_Fire);
	BlockAbilitiesWithTag.AddTag(FPSGameplayTags::Ability_Combat_Reload);
	CancelAbilitiesWithTag.AddTag(FPSGameplayTags::Ability_Combat_Fire);
	DamageEffectClass = UFPSMeleeDamageEffect::StaticClass();
}

bool UFPSMeleeAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (nullptr == ActorInfo || false == ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	const ACharacterPlayer* Character = Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get());
	if (false == IsValid(Character) || true == Character->IsDeathStarted()
		|| nullptr != Character->GetEquippedGrenade())
	{
		return false;
	}

	if (true == ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(FPSGameplayTags::Status_Melee)
		|| true == Attacks.IsEmpty())
	{
		return false;
	}

	if (false == IsValid(Attacks[0].FirstPersonMontage) || false == IsValid(Attacks[0].ThirdPersonMontage))
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UFPSMeleeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	_Ending = false;
	ACharacterPlayer* Character = nullptr != ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (false == IsValid(Character) || false == CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetAiming(false);

	// 몽타주 재생 전에 이벤트를 등록하여 첫 프레임의 노티파이도 받는다.
	_EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FPSGameplayTags::Event_Melee, nullptr, false, false);
	_EventTask->EventReceived.AddDynamic(this, &ThisClass::OnMeleeEvent);
	_EventTask->ReadyForActivation();

	UAbilitySystemComponent* AbilitySystem = ActorInfo->AbilitySystemComponent.Get();
	for (const FGameplayTag& Tag : ActivationBlockedTags)
	{
		_BlockedTagDelegates.Add(Tag,
			AbilitySystem->RegisterGameplayTagEvent(Tag).AddUObject(this, &ThisClass::OnBlockedTagChanged));
	}

	StartAttack(0);
	if (true == IsActive())
	{
		WaitForComboInput();
	}
}

void UFPSMeleeAbility::StartAttack(int32 Index)
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());
	if (false == IsActive() || false == IsValid(Character) || true == Character->IsDeathStarted()
		|| false == Attacks.IsValidIndex(Index))
	{
		FailAttack();
		return;
	}

	const FFPSMeleeAttack& Attack = Attacks[Index];
	if (false == IsValid(Attack.ThirdPersonMontage) || false == IsValid(Attack.FirstPersonMontage)
		|| Attack.PlayRate <= 0.f)
	{
		FailAttack();
		return;
	}

	StopHitWindow();
	ComboIndex = Index;
	bBufferedInput = false;
	bComboWindowOpen = false;
	_InputWindowClosed = false;
	_HitActors.Reset();

	// 1인칭 팔은 소유 클라이언트에서만 재생한다. 몸 몽타주는 GAS가 복제한다.
	if (true == Character->IsLocallyControlled())
	{
		USkeletalMeshComponent* Arms = Character->Get_FirstPersonMesh();
		UAnimInstance* AnimInstance = true == IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;
		_PlayingFirstPersonMontage = Attack.FirstPersonMontage;

		if (false == IsValid(AnimInstance) || AnimInstance->Montage_Play(_PlayingFirstPersonMontage,
			Attack.PlayRate, EMontagePlayReturnType::MontageLength, 0.f, false) <= 0.f)
		{
			FailAttack();
			return;
		}
	}

	// 종료 콜백이 누락되어도 몽타주 길이 이후에는 공격 상태를 정리한다.
	GetWorld()->GetTimerManager().SetTimer(_WatchdogTimer, this, &ThisClass::FailAttack,
		Attack.ThirdPersonMontage->GetPlayLength() / Attack.PlayRate + 0.5f, false);

	_MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None,
		Attack.ThirdPersonMontage, Attack.PlayRate, NAME_None, true, 0.f, 0.f, true);

	_MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAttackCompleted);
	_MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAttackInterrupted);
	_MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAttackInterrupted);

	_MontageTask->ReadyForActivation();
}

void UFPSMeleeAbility::WaitForComboInput()
{
	// 이미 눌린 상태를 다시 소비하지 않으므로 키를 길게 눌러도 자동으로 이어지지 않는다.
	_InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	_InputTask->OnPress.AddDynamic(this, &ThisClass::OnComboInput);
	_InputTask->ReadyForActivation();
}

void UFPSMeleeAbility::OnComboInput(float TimeWaited)
{
	if (false == IsActive() || true == _Ending)
	{
		return;
	}

	// 콤보 종료 노티파이 전의 추가 입력 하나로 다음 공격을 예약한다.
	if (false == _InputWindowClosed && true == Attacks.IsValidIndex(ComboIndex + 1))
	{
		bBufferedInput = true;
	}

	WaitForComboInput();
}

void UFPSMeleeAbility::OnMeleeEvent(FGameplayEventData Payload)
{
	// 이전 단계의 몽타주가 블렌딩 중 보낸 이벤트는 현재 공격에 적용하지 않는다.
	if (false == IsActive() || true == _Ending || FMath::RoundToInt(Payload.EventMagnitude) != ComboIndex)
	{
		return;
	}

	if (Payload.EventTag == FPSGameplayTags::Event_Melee_ComboOpen)
	{
		bComboWindowOpen = true;
	}
	else if (Payload.EventTag == FPSGameplayTags::Event_Melee_ComboClose)
	{
		bComboWindowOpen = false;
		_InputWindowClosed = true;
	}
	else if (Payload.EventTag == FPSGameplayTags::Event_Melee_HitStart
		&& true == GetCurrentActorInfo()->IsNetAuthority())
	{
		// 피해 판정은 서버의 타격 구간에서만 반복한다. 시작 프레임도 즉시 검사한다.
		GetWorld()->GetTimerManager().SetTimer(_HitTimer, this, &ThisClass::SweepForTargets, 1.f / 60.f, true);
		SweepForTargets();
	}
	else if (Payload.EventTag == FPSGameplayTags::Event_Melee_HitEnd)
	{
		StopHitWindow();
	}
}

void UFPSMeleeAbility::StopHitWindow()
{
	UWorld* World = GetWorld();
	if (nullptr != World)
	{
		World->GetTimerManager().ClearTimer(_HitTimer);
	}
}

void UFPSMeleeAbility::SweepForTargets()
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());

	UFPSAbilitySystemComponent* AbilitySystem = Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());

	if (false == IsValid(Character) || false == Character->HasAuthority() || false == IsValid(AbilitySystem)
		|| false == AbilitySystem->CanAttack() || true == Character->IsDeathStarted())
	{
		FailAttack();
		return;
	}

	if (false == Attacks.IsValidIndex(ComboIndex) || nullptr == DamageEffectClass)
	{
		return;
	}

	FVector		EyeLocation;
	FRotator	EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector Forward	= EyeRotation.Vector();
	const FVector Start		= EyeLocation - FVector(0.f, 0.f, 20.f) + Forward * 20.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Melee), false, Character);

	if (true == IsValid(Character->GetEquippedWeapon()))
	{
		Params.AddIgnoredActor(Character->GetEquippedWeapon());
	}

	// 캐릭터 캡슐은 Visibility를 무시하므로 Pawn 객체로 대상을 찾는다.
	// 벽 차단은 별도 Visibility 검사로 처리하여 벽 너머의 적을 타격하지 않는다.

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, Start + Forward * Reach, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(SweepRadius), Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();

		if (false == IsValid(Target) || true == _HitActors.Contains(Target))
		{
			continue;
		}

		UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

		if (false == IsValid(TargetAbilitySystem)
			|| false == TargetAbilitySystem->HasAttributeSetForAttribute(UFPSHealthSet::Get_DamageInAttribute()))
		{
			continue;
		}

		FHitResult Blocker;

		// ECC_Visibility : 캐릭터의 시야 체크, 라인 트레이스(Line Trace), 오브젝트 간 시선 차단 여부
		if (true == GetWorld()->SweepSingleByChannel(Blocker, Start, Hit.Location, FQuat::Identity,
			ECC_Visibility, FCollisionShape::MakeSphere(SweepRadius), Params)
			&& Blocker.GetActor() != Target
			&& (nullptr == Blocker.GetActor() || Blocker.GetActor()->GetOwner() != Target))
		{
			continue;
		}

		// 한 공격 단계에서 동일한 대상에게 피해가 중복 적용되지 않도록 기록한다.
		_HitActors.Add(Target);
		FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
		Context.AddInstigator(Character, Character);
		Context.AddHitResult(Hit, true);
		FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);

		if (true == Spec.IsValid())
		{
			// 피해량은 단계별 설정을 사용하고 체력·실드 처리는 기존 HealthSet에 맡긴다.
			Spec.Data->SetSetByCallerMagnitude(FPSGameplayTags::SetByCaller_Damage, Attacks[ComboIndex].Damage);
			AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetAbilitySystem);
		}
	}
}

void UFPSMeleeAbility::OnAttackCompleted()
{
	if (false == IsActive() || true == _Ending)
	{
		return;
	}

	StopHitWindow();
	GetWorld()->GetTimerManager().ClearTimer(_WatchdogTimer);
	if (true == bBufferedInput && true == Attacks.IsValidIndex(ComboIndex + 1))
	{
		StartAttack(ComboIndex + 1);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UFPSMeleeAbility::OnAttackInterrupted()
{
	FailAttack();
}

void UFPSMeleeAbility::FailAttack()
{
	if (true == IsActive() && false == _Ending)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UFPSMeleeAbility::OnBlockedTagChanged(FGameplayTag Tag, int32 Count)
{
	// 사망이나 전투 제한 등의 태그가 도중에 부여되면 콤보도 즉시 취소한다.
	if (Count > 0)
	{
		FailAttack();
	}
}

void UFPSMeleeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (true == _Ending)
	{
		return;
	}

	_Ending = true;
	StopHitWindow();

	UWorld* World = GetWorld();
	if (nullptr != World)
	{
		World->GetTimerManager().ClearTimer(_WatchdogTimer);
	}

	if (nullptr != ActorInfo && true == ActorInfo->AbilitySystemComponent.IsValid())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Pair : _BlockedTagDelegates)
		{
			ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(Pair.Key).Remove(Pair.Value);
		}
	}

	_BlockedTagDelegates.Reset();

	ACharacterPlayer* Character = nullptr != ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (true == IsValid(Character))
	{
		USkeletalMeshComponent* Arms = Character->Get_FirstPersonMesh();
		UAnimInstance* AnimInstance = true == IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;
		if (true == IsValid(AnimInstance) && true == IsValid(_PlayingFirstPersonMontage) && true == bWasCancelled)
		{
			AnimInstance->Montage_Stop(0.1f, _PlayingFirstPersonMontage);
		}
	}

	_HitActors.Reset();
	bBufferedInput = false;
	bComboWindowOpen = false;
	ComboIndex = INDEX_NONE;
	_PlayingFirstPersonMontage = nullptr;

	// 정상 종료와 취소 모두 GAS의 태스크·활성화 소유 태그 정리를 거친다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	_MontageTask = nullptr;
	_InputTask = nullptr;
	_EventTask = nullptr;
}
