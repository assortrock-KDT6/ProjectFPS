#include "Character/CharacterBase.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "GameTag/FPSGameplayTag.h"
#include "GameFramework/CharacterMovementComponent.h"

ACharacterBase::ACharacterBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// TODO : private 함수로 빼두기

	PrimaryActorTick.bCanEverTick = true;

	// Ability System Component를 생성하고 명시적으로 복제되도록 설정한다.
	_AbilitySystemComponent = CreateDefaultSubobject<UFPSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	_AbilitySystemComponent->SetIsReplicated(true);
	_AbilitySystemComponent->SetReplicationMode(_AbilitySystemComponentReplicationMode);
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshMovementTags();
	
}

void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ACharacterBase::PossessedBy(AController* Newcontroller)
{
	Super::PossessedBy(Newcontroller);

	if (nullptr != _AbilitySystemComponent)
	{
		_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

}

void ACharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (nullptr != _AbilitySystemComponent)
	{
		_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

const FName& ACharacterBase::GetId() const
{
	return _Id;
}

void ACharacterBase::SetId(const FName& Id)
{
	_Id = Id;
}

UAbilitySystemComponent* ACharacterBase::GetAbilitySystemComponent() const
{
	return _AbilitySystemComponent;
}

void ACharacterBase::SetAnimationStateTag(FGameplayTag Tag, bool bActive)
{
	if (!HasAuthority() || !IsValid(_AbilitySystemComponent) || !Tag.IsValid())
	{
		return;
	}

	_AbilitySystemComponent->SetLooseGameplayTagCount(Tag, bActive ? 1 : 0, EGameplayTagReplicationState::TagAndCountToAll);
	ForceNetUpdate();
}

void ACharacterBase::RefreshMovementTags()
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (false == IsValid(_AbilitySystemComponent) || nullptr == Movement)
	{
		return;
	}

	for (const auto& Pair : FPSGameplayTags::MovementModeTagMap)
	{
		_AbilitySystemComponent->SetLooseGameplayTagCount(Pair.Value, Movement->MovementMode == Pair.Key ? 1 : 0);
	}

	for (const auto& Pair : FPSGameplayTags::CustomMovementModeTagMap)
	{
		_AbilitySystemComponent->SetLooseGameplayTagCount(Pair.Value,
			Movement->MovementMode == MOVE_Custom && Movement->CustomMovementMode == Pair.Key ? 1 : 0);
	}

	_AbilitySystemComponent->SetLooseGameplayTagCount(FPSGameplayTags::Status_Crouching, bIsCrouched ? 1 : 0);
}

void ACharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	RefreshMovementTags();
}

void ACharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	RefreshMovementTags();
}

void ACharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	RefreshMovementTags();
}

