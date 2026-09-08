#include "Character/CharacterBase.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"

ACharacterBase::ACharacterBase(const FObjectInitializer& ObjectInitializer)
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

