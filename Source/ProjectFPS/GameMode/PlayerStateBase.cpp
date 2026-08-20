// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/PlayerStateBase.h"

void APlayerStateBase::BeginPlay()
{
	Super::BeginPlay();

	_Items = _StartItems; 
}

void APlayerStateBase::AddItem(FName TID, int32 Count)
{
	if (TID.IsNone() || Count <= 0)
		return;
	int32& Cur = _Items.FindOrAdd(TID);
	Cur += Count;
}
