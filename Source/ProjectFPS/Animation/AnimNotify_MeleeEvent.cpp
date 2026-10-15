#include "Animation/AnimNotify_MeleeEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/CharacterPlayer.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_MeleeEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	ACharacterPlayer* Character = true == IsValid(MeshComp) ? Cast<ACharacterPlayer>(MeshComp->GetOwner()) : nullptr;
	if (false == IsValid(Character) || MeshComp != Character->GetMesh() || false == EventTag.IsValid())
	{
		return;
	}

	// 1인칭 팔의 중복 이벤트와 다른 플레이어의 표시용 이벤트를 제외한다.
	// 서버는 타격을 처리하고 소유 클라이언트는 콤보 진행을 예측한다.
	if (false == Character->HasAuthority() && false == Character->IsLocallyControlled())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.EventMagnitude = AttackIndex;
	Payload.Instigator = Character;
	Payload.Target = Character;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, EventTag, Payload);
}
