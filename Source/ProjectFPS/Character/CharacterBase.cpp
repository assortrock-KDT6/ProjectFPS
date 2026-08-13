#include "Character/CharacterBase.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

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

}

const FName& ACharacterBase::GetId() const
{
	return _Id;
}

void ACharacterBase::SetId(const FName& Id)
{
	_Id = Id;
}

