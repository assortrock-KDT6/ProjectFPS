// Fill out your copyright notice in the Description page of Project Settings.
#include "Component/Ability/GamePlayAbility/FPSGrenadeAbility.h"
#include "GameMode/PlayerStateBase.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Character/CharacterPlayer.h"
#include "GameTag/FPSGameplayTag.h"
#include "Projectiles/GrenadeActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "UniversalObjectLocators/AnimInstanceLocatorFragment.h"

UFPSGrenadeAbility::UFPSGrenadeAbility()
{
	// 수류탄 능력의 게임 판정은 서버에서 실행한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	// 사망, 전투 금지 검사와  InstancingPolicy는 FPSCombatAbility 생성자에서 설정
	// 파쿠르, 재장전, 근접 공격 중에는 투척을 시작하지 못하게
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Vault);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Movement_Mode_Mantle);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Reloading);
	ActivationBlockedTags.AddTag(FPSGameplayTags::Status_Melee);
}

void UFPSGrenadeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
										 const FGameplayAbilityActorInfo* ActorInfo, 
										 const FGameplayAbilityActivationInfo ActivationInfo,
										 const FGameplayEventData* TriggerEventData)
{
	ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
	_ThirdPersonMontageStarted = false;
	
	if (false == IsValid(Character) ||
		false == Character->HasAuthority() ||
		false == IsValid(Character->GetEquippedGrenade()) ||
		false == IsValid(_GrenadeMontageFP))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	USkeletalMeshComponent* Arms = Character->Get_FirstPersonMesh();
	UAnimInstance* AnimInstance = IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;
	
	if (false == IsValid(AnimInstance) ||
		_GrenadeMontageFP->GetSectionIndex(TEXT("Ready")) == INDEX_NONE ||
		_GrenadeMontageFP->GetSectionIndex(TEXT("ReadyHold")) == INDEX_NONE ||
		_GrenadeMontageFP->GetSectionIndex(TEXT("Throw")) == INDEX_NONE ||
		false == CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// 서버에서도 팔의 본 위치와 몽타주 노티파이를 갱신
	Arms->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	
	AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UFPSGrenadeAbility::OnGrenadeRelease);
	
	// 서버도 재생하여 몽타주의 진행과 종료 시점을 판단
	// 장착 시 멈춰 둔 몽타주가 없다면 새로 재생한다.
	if (!AnimInstance->Montage_IsActive(_GrenadeMontageFP))
	{
		const float Duration = AnimInstance->Montage_Play(_GrenadeMontageFP, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false);

		if (Duration <= 0.f)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			
			return;
		}
	}

	AnimInstance->Montage_JumpToSection(TEXT("Ready"), _GrenadeMontageFP);
	
	AnimInstance->Montage_Resume(_GrenadeMontageFP);
	
	FOnMontageEnded EndDelegate;
	
	EndDelegate.BindUObject(this, &UFPSGrenadeAbility::OnMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(EndDelegate, _GrenadeMontageFP);
	PlayThirdPersonMontage(TEXT("Ready"));
	
	// 기존 ASC가 전달하는 Cook Event를 대기
	UAbilityTask_WaitGameplayEvent* CookTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FPSGameplayTags::Event_Grenade_Cook, nullptr, false, true);
	
	CookTask->EventReceived.AddDynamic(this, &UFPSGrenadeAbility::OnGrenadeCook);
	
	CookTask->ReadyForActivation();
	
	// 준비 중에도 사망, 전투 금지, 파쿠르등의 차단 상태를 감시
	for (const FGameplayTag& Tag : ActivationBlockedTags)
	{
		UAbilityTask_WaitGameplayTagAdded* Task = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, Tag, nullptr, true);
		
		Task->Added.AddDynamic(static_cast<UGameplayAbility*>(this), &UGameplayAbility::K2_CancelAbility);
		
		Task->ReadyForActivation();
		
		if (false == IsActive())
		{
			return;
		}
	}
	
	// 리슨 서버의 로컬 플레이어는 위에서 이미 재생했음
	if (false == Character->IsLocallyControlled())
	{
		Character->ClientPlayGrenadeMontage(_GrenadeMontageFP, TEXT("Ready"));
	}
}

void UFPSGrenadeAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
									   const FGameplayAbilityActorInfo* ActorInfo, 
									   const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (false == IsActive())
	{
		return;
	}

	ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (false == IsValid(Character) || false == Character->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		
		return;
	}

	USkeletalMeshComponent* Arms = Character->Get_FirstPersonMesh();
	
	UAnimInstance* AnimInstance  = IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;

	if (false == IsValid(AnimInstance) || false == AnimInstance->Montage_IsPlaying(_GrenadeMontageFP))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		
		return;
	}

	// 이미 던지는 중이라면 반복 입력 무시
	if (AnimInstance->Montage_GetCurrentSection(_GrenadeMontageFP) == FName(TEXT("Throw")))
	{
		return;
	}

	// Throw를 전환하기 전에 장착한 수류탄을 확인
	if (false == IsValid(Character->GetEquippedGrenade()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		
		return;
	}
	
	AnimInstance->Montage_JumpToSection(TEXT("Throw"), _GrenadeMontageFP);
	PlayThirdPersonMontage(TEXT("Throw"));

	if (false == Character->IsLocallyControlled())
	{
		Character->ClientPlayGrenadeMontage(_GrenadeMontageFP, TEXT("Throw"));
	}
}

void UFPSGrenadeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, 
									const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayAbilityActivationInfo ActivationInfo, 
									bool bReplicateEndAbility, 
									bool bWasCancelled)
{
	if (false == IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	ACharacterPlayer* Character = ActorInfo ? Cast<ACharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	USkeletalMeshComponent* Arms = IsValid(Character) ? Character->Get_FirstPersonMesh() : nullptr;

	UAnimInstance* AnimInstance = IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;

	if (IsValid(AnimInstance) && IsValid(_GrenadeMontageFP))
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UFPSGrenadeAbility::OnGrenadeRelease);
		
		// 정지하면서 종료 콜백이 다시 실행되지 않도록 먼저 해제한다
		if (FOnMontageEnded* EndDelegate = AnimInstance->Montage_GetEndedDelegate(_GrenadeMontageFP))
		{
			if (EndDelegate->IsBoundToObject(this))
			{
				EndDelegate->Unbind();
			}
		}
	}
	
	// 능력이 끝날 때 아직 들고 있는 쿠킹 수류탄을 떨어뜨린다
	if (IsValid(Character) && Character->HasAuthority())
	{
		AGrenadeActor* Grenade = Character->GetEquippedGrenade();
		
		if (IsValid(Grenade) && Grenade->GetLifeSpan() > 0.f && Grenade->Drop())
		{
			Character->ClearGrenadeReference(Grenade);
		}
	}
	
	if (bWasCancelled && IsValid(AnimInstance) && IsValid(_GrenadeMontageFP))
	{
		AnimInstance->Montage_Stop(_GrenadeMontageFP->GetDefaultBlendOutTime(), _GrenadeMontageFP);
	}
	
	if (bWasCancelled && IsValid(Character) && !Character->IsLocallyControlled() && IsValid(_GrenadeMontageFP))
	{
		Character->ClientStopGrenadeMontage(_GrenadeMontageFP);
	}

	if (_ThirdPersonMontageStarted)
	{
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (IsValid(ASC))
		{
			if (bWasCancelled && ASC->GetAnimatingAbility() == this && ASC->GetCurrentMontage() == _GrenadeMontageTP)
			{
				ASC->CurrentMontageStop(_GrenadeMontageTP->GetDefaultBlendOutTime());
			}
			// 정상 종료는 TP의 남은 블렌드 아웃을 그대로 두고 소유 관계만 해제한다.
			ASC->ClearAnimatingAbility(this);
		}
		if (bWasCancelled && IsValid(Character) && !Character->IsLocallyControlled())
		{
			Character->ClientStopGrenadeMontageTP(_GrenadeMontageTP);
		}
		_ThirdPersonMontageStarted = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFPSGrenadeAbility::PlayThirdPersonMontage(FName Section)
{
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(Character) || !Character->HasAuthority() || !IsValid(ASC) || !IsValid(_GrenadeMontageTP))
	{
		return;
	}

	if (Section == TEXT("Ready"))
	{
		if (!IsValid(Character->GetMesh()) || !IsValid(Character->GetMesh()->GetAnimInstance())
			|| _GrenadeMontageTP->GetSectionIndex(TEXT("Ready")) == INDEX_NONE
			|| _GrenadeMontageTP->GetSectionIndex(TEXT("ReadyHold")) == INDEX_NONE
			|| _GrenadeMontageTP->GetSectionIndex(TEXT("Throw")) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grenade TP montage requires a body AnimInstance and Ready/ReadyHold/Throw sections."));
			return;
		}
		_ThirdPersonMontageStarted = ASC->PlayMontage(this, CurrentActivationInfo, _GrenadeMontageTP, 1.f, Section) > 0.f;
	}
	else if (_ThirdPersonMontageStarted && ASC->GetAnimatingAbility() == this && ASC->GetCurrentMontage() == _GrenadeMontageTP)
	{
		ASC->CurrentMontageJumpToSection(Section);
	}
	else
	{
		return;
	}

	// GAS 몽타주 복제는 simulated proxy에 적용된다. 소유 클라이언트는 따로 표시한다.
	if (_ThirdPersonMontageStarted && !Character->IsLocallyControlled())
	{
		Character->ClientPlayGrenadeMontageTP(_GrenadeMontageTP, Section);
	}
}

UAnimMontage* UFPSGrenadeAbility::GetGrenadeMontageFP() const
{
	return _GrenadeMontageFP.Get();
}

UAnimMontage* UFPSGrenadeAbility::GetGrenadeMontageTP() const
{
	return _GrenadeMontageTP.Get();
}

void UFPSGrenadeAbility::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (false == IsActive() || Montage != _GrenadeMontageFP)
	{
		return;
	}

	ACharacterPlayer* Character  = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());

	USkeletalMeshComponent* Arms = IsValid(Character) ? Character->Get_FirstPersonMesh() : nullptr;

	UAnimInstance* AnimInstance  = IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;

	// 이전 재생의 종료 알림이 새 재생을 종료시키지 않도록 한다
	if (IsValid(AnimInstance) && AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	
	// 정상 투척이 끝났으면 기존 장착 경로로 다음 수류탄을 장착
	// 기존 ServerEquipGrenade() 가 서버 인벤토리 수량을 검사하고 남은 수량이 없으면 장착하지 않음
	// 이미 장착했던 총으로 돌아가고 총도없으면 맨손상태
	if (!bInterrupted && IsValid(Character) && !IsValid(Character->GetEquippedGrenade()))
	{
		Character->ServerEquipGrenade();
	}
}

void UFPSGrenadeAbility::OnGrenadeRelease(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	if (NotifyName != FName(TEXT("GrenadeRelease")) || false == IsActive())
	{
		return;
	}
	
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());
	
	if (false == IsValid(Character) || false == Character->HasAuthority())
	{
		return;
	}
	
	USkeletalMeshComponent* Arms = Character->Get_FirstPersonMesh();
	
	UAnimInstance* AnimInstance = IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;
	
	if (false == IsValid(AnimInstance) || Payload.SkelMeshComponent != Arms || false == Arms->DoesSocketExist(TEXT("GrenadeGrip")))
	{
		return;
	}
	
	const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(_GrenadeMontageFP);
	
	// 다른 몽타주나 이전 재생에서 발생한 노티파이는 안받는다
	if (nullptr == MontageInstance || MontageInstance->GetInstanceID() != Payload.MontageInstanceID || AnimInstance->Montage_GetCurrentSection(_GrenadeMontageFP) != FName(TEXT("Throw")))
	{
		return;
	}
	
	AGrenadeActor* Grenade = Character->GetEquippedGrenade();
	
	if (false == IsValid(Grenade))
	{
		return;
	}
	
	// const FVector ReleaseLocation = Arms->GetSocketLocation(TEXT("GrenadeGrip"));
	//
	// const FVector Direction = Character->GetControlRotation().Vector();
	//
	// if (Grenade->Throw(Direction))
	// {
	// 	// 1인칭 손의 위치에서 출발 -> 적용된 물리 속도는 유지
	// 	Grenade->SetActorLocation(ReleaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
	// 	
	// 	Character->ClearGrenadeReference(Grenade);
	// }

	// APlayerStateBase* FPSPlayerState = Character->GetPlayerState<APlayerStateBase>();
	//
	// UInventoryComponent* Inventory = IsValid(FPSPlayerState) ? FPSPlayerState->GetInventory() : nullptr;
	//
	// if (false == IsValid(Inventory) || false == IsValid(Inventory->GetOwner()) || false == Inventory->GetOwner()->HasAuthority())
	// {
	// 	return;
	// }
	//
	// // 장착한 수류탄과 같은 아이템이 들어 있는 슬롯을 찾기
	// const TArray<FInventorySlot>& Items = Inventory->GetItems();
	//
	// int32 SlotIndex = INDEX_NONE;
	//
	// for (int32 Index = 0; Index < Items.Num(); ++Index)
	// {
	// 	if (Items[Index]._TID == Grenade->GetTID() && Items[Index]._Count > 0)
	// 	{
	// 		SlotIndex = Index;
	// 		
	// 		break;
	// 	}
	// }
	//
	// if (SlotIndex == INDEX_NONE)
	// {
	// 	return;
	// }
	//
	// const FVector ReleaseLocation = Arms->GetSocketLocation(TEXT("GrenadeGrip"));
	//
	// const FVector Direction = Character->GetControlRotation().Vector();
	//
	// if (Grenade->Throw(Direction))
	// {
	// 	// 실제 투척에 성공했을때 서버 인벤토리에서 한개 소모
	// 	Inventory->RemoveItem(SlotIndex, 1);
	// 	
	// 	// 1인칭 손의 위치에서 출발 -> 적용된 물리 속도는 유지
	// 	Grenade->SetActorLocation(ReleaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
	//
	// 	Character->ClearGrenadeReference(Grenade);
	// }
	
	APlayerStateBase* FPSPlayerState = Character->GetPlayerState<APlayerStateBase>();
	
	UInventoryComponent* Inventory = IsValid(FPSPlayerState) ? FPSPlayerState->GetInventory() : nullptr;
	
	if (!IsValid(Inventory) || !IsValid(Inventory->GetOwner()) || !Inventory->GetOwner()->HasAuthority())
	{
		return;
	}
	
	int32 SlotIndex = INDEX_NONE;
	
	// 쿠킹하지 않은 수류탄 투척할 떄, 소비할 슬롯을 갖는다
	if (Grenade->GetLifeSpan() <= 0.f)
	{
		const TArray<FInventorySlot>& Items = Inventory->GetItems();
		
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index]._TID == Grenade->GetTID() && Items[Index]._Count > 0)
			{
				SlotIndex = Index;
				
				break;
			}
		}
		
		if (SlotIndex == INDEX_NONE)
		{
			return;
		}
	}
	
	const FVector ReleaseLocation = Arms->GetSocketLocation(TEXT("GrenadeGrip"));
	
	const FVector Direction = Character->GetControlRotation().Vector();
	
	if (Grenade->Throw(Direction))
	{
		// 쿠킹할 것을 이미 소비했으니까 추가로 소비하지않게
		if (SlotIndex != INDEX_NONE)
		{
			Inventory->RemoveItem(SlotIndex, 1);
		}
		
		Grenade->SetActorLocation(ReleaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
		
		Character->ClearGrenadeReference(Grenade);
	}
	
}

void UFPSGrenadeAbility::OnGrenadeCook(FGameplayEventData Payload)
{
	if (!IsActive() || Payload.EventTag != FPSGameplayTags::Event_Grenade_Cook)
	{
		return;
	}
	
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(GetAvatarActorFromActorInfo());
	
	if (!IsValid(Character) || !Character->HasAuthority() || !IsValid(_GrenadeMontageFP))
	{
		return;
	}
	
	USkeletalMeshComponent* Arms = Character->Get_FirstPersonMesh();
	
	UAnimInstance* AnimInstance = IsValid(Arms) ? Arms->GetAnimInstance() : nullptr;
	
	if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsPlaying(_GrenadeMontageFP))
	{
		return;
	}
	
	const FName Section = AnimInstance->Montage_GetCurrentSection(_GrenadeMontageFP);
	
	if (Section != FName(TEXT("Ready")) && Section != FName(TEXT("ReadyHold")))
	{
		return;
	}
	
	AGrenadeActor* Grenade = Character->GetEquippedGrenade();
	
	// 이미 쿠킹 중이라면 남은 시간을 다시 5초로 되돌리지 않음
	if (!IsValid(Grenade) || Grenade->GetLifeSpan() > 0.f)
	{
		return;
	}
	
	APlayerStateBase* FPSPlayerState = Character->GetPlayerState<APlayerStateBase>();
	
	UInventoryComponent* Inventory = IsValid(FPSPlayerState) ? FPSPlayerState->GetInventory() : nullptr;
	
	if (!IsValid(Inventory) || !IsValid(Inventory->GetOwner()) || !Inventory->GetOwner()->HasAuthority())
	{
		return;
	}
	
	const TArray<FInventorySlot>& Items = Inventory->GetItems(); // Inventory에서 수류탄 남은거 있는지 TID 찾고 쿠킹하면 -1할 목적을 가짐
	
	for (int32 Index = 0; Index < Items.Num(); ++ Index)
	{
		if (Items[Index]._TID == Grenade->GetTID() && Items[Index]._Count > 0)
		{
			// 쿠킹이 시작된 수류탄은 이때 한번만 소비
			if (Inventory->RemoveItem(Index, 1))
			{
				Grenade->SetLifeSpan(5.f);
			}
			
			return;
		}
	}
}
