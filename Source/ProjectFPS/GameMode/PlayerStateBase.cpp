// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/PlayerStateBase.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

APlayerStateBase::APlayerStateBase()
{
	_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void APlayerStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerStateBase, _bIsDead);
}

bool APlayerStateBase::IsDead() const
{
	return _bIsDead;
}

void APlayerStateBase::SetDead(const bool bIsDead)
{
	_bIsDead = bIsDead;
}
	