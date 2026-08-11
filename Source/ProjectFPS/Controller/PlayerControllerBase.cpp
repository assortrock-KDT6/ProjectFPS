// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/PlayerControllerBase.h"

APlayerControllerBase::APlayerControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void APlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();
}