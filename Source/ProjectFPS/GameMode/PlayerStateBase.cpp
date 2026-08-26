// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/PlayerStateBase.h"
#include "Component/Inventory/InventoryComponent.h"

APlayerStateBase::APlayerStateBase()
{
	_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}
