#include "Animation/FPSMeleeAnimationLibrary.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNotify_MeleeEvent.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "Component/Ability/GamePlayAbility/FPSMeleeAbility.h"
#include "Engine/World.h"
#include "GameTag/FPSGameplayTag.h"
#include "TimerManager.h"

bool UFPSMeleeAnimationLibrary::ConfigureMeleeMontage(UAnimMontage* Montage, UAnimSequence* Sequence, FName SlotName,
	int32 AttackIndex, bool bFirstPerson, float HitStartFraction, float HitEndFraction)
{
#if WITH_EDITOR
	if (false == IsValid(Montage) || false == IsValid(Sequence) || true == SlotName.IsNone()
		|| AttackIndex < 0 || HitStartFraction < 0.f
		|| HitEndFraction <= HitStartFraction || HitEndFraction >= 0.8f)
	{
		return false;
	}

	// 반복 호출해도 이전 구성과 중복되지 않도록 슬롯과 구간을 다시 만든다.
	Montage->Modify();
	Montage->SetSkeleton(Sequence->GetSkeleton());
	Montage->SlotAnimTracks.Reset();
	FSlotAnimationTrack& Slot = Montage->SlotAnimTracks.AddDefaulted_GetRef();
	Slot.SlotName = SlotName;
	FAnimSegment& Segment = Slot.AnimTrack.AnimSegments.AddDefaulted_GetRef();
	Segment.SetAnimReference(Sequence, true);
	const float Length = Sequence->GetPlayLength();
	Segment.AnimEndTime = Length;
	Segment.AnimPlayRate = 1.f;
	Segment.LoopingCount = 1;
	Montage->SetCompositeLength(Length);
	Montage->BlendIn.SetBlendTime(0.08f);
	Montage->BlendOut.SetBlendTime(0.08f);
	Montage->CompositeSections.Reset();
	FCompositeSection& Section = Montage->CompositeSections.AddDefaulted_GetRef();
	Section.SectionName = TEXT("Attack");
	Section.SetTime(0.f);

	// 판정 이벤트는 몸 몽타주에만 넣어 1인칭 팔과 중복되지 않도록 한다.
	Montage->Notifies.Reset();
	if (false == bFirstPerson)
	{
		const TPair<FGameplayTag, float> Events[] =
		{
			{ FPSGameplayTags::Event_Melee_HitStart, HitStartFraction },
			{ FPSGameplayTags::Event_Melee_HitEnd, HitEndFraction },
			{ FPSGameplayTags::Event_Melee_ComboOpen, 0.4f },
			{ FPSGameplayTags::Event_Melee_ComboClose, 0.8f }
		};

		for (const TPair<FGameplayTag, float>& Event : Events)
		{
			FAnimNotifyEvent& Notify = Montage->Notifies.AddDefaulted_GetRef();
			UAnimNotify_MeleeEvent* MeleeNotify = NewObject<UAnimNotify_MeleeEvent>(Montage);
			MeleeNotify->EventTag = Event.Key;
			MeleeNotify->AttackIndex = AttackIndex;
			Notify.Notify = MeleeNotify;
			Notify.NotifyName = Event.Key.GetTagName();

			// 전용 서버에서도 타격 구간을 평가하며, 몽타주 갱신 시점에 즉시 전달한다.
			Notify.bTriggerOnDedicatedServer = true;
			Notify.MontageTickType = EMontageNotifyTickType::BranchingPoint;
			Notify.Link(Montage, Length * Event.Value);
		}
	}

	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UFPSMeleeAnimationLibrary::QueuePIEMeleeInput(UFPSAbilitySystemComponent* AbilitySystem, bool Pressed, bool Cancel)
{
#if WITH_EDITOR
	if (false == IsValid(AbilitySystem))
	{
		return false;
	}

	UWorld* World = AbilitySystem->GetWorld();
	if (false == IsValid(World) || World->WorldType != EWorldType::PIE)
	{
		return false;
	}

	// Python 실행 중에는 RPC가 로컬 호출로 처리된다. 가드가 해제된 다음 틱에서 전송한다.
	// 대기 중 PIE가 종료되어도 컴포넌트를 붙잡지 않도록 약한 참조를 사용한다.
	TWeakObjectPtr<UFPSAbilitySystemComponent> WeakAbilitySystem(AbilitySystem);
	World->GetTimerManager().SetTimerForNextTick([WeakAbilitySystem, Pressed, Cancel]()
	{
		UFPSAbilitySystemComponent* Component = WeakAbilitySystem.Get();
		if (nullptr != Component)
		{
			if (true == Cancel)
			{
				UFPSMeleeAbility* Ability = Component->GetMeleeAbilityInstance();
				if (nullptr != Ability)
				{
					Ability->K2_CancelAbility();
				}
			}
			else
			{
				Component->SetMeleeInput(Pressed);
			}
		}
	});

	return true;
#else
	return false;
#endif
}
