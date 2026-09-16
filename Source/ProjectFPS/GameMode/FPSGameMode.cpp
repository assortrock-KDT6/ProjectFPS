// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/FPSGameMode.h"
#include "GameMode/PlayerStateBase.h"
#include "GameMode/FPSGameState.h"
#include "Pawn/FPSSpectatorPawn.h"
#include "Character/CharacterPlayer.h"
#include "Controller/PlayerControllerBase.h"

AFPSGameMode::AFPSGameMode()
{
	SpectatorClass = AFPSSpectatorPawn::StaticClass();
}

void AFPSGameMode::HandlePlayerDeath(ACharacterPlayer* DeadCharacter)
{
	if (false == HasAuthority() || false == IsValid(DeadCharacter))
	{
		return;
	}

	APlayerControllerBase* PlayerController = Cast<APlayerControllerBase>(DeadCharacter->GetController());
	if (false == IsValid(PlayerController))
	{
		return;
	}

	FVector		CameraLocation;
	FRotator	CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	UCharacterMovementComponent* Movement = DeadCharacter->GetCharacterMovement();
	if (true == IsValid(Movement))
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	DeadCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeadCharacter->DetachFromControllerPendingDestroy();

	PlayerController->EnterDeathSpectating(CameraLocation, CameraRotation);

	PlayerController->StartSpectatingOnly();

	APlayerStateBase* PlayerState = Cast<APlayerStateBase>(DeadCharacter->GetPlayerState());
	if (true == IsValid(PlayerState))
	{
		PlayerState->SetDead(true);
	}
	
	DeadCharacter->SetLifeSpan(5.f);

	CheckMatchEnd();
}

void AFPSGameMode::CheckMatchEnd()
{
	int32 AlivePlayerCount = 0;
	APlayerStateBase* LastAlivePlayer = nullptr;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		APlayerStateBase* FPSPlayerState = Cast<APlayerStateBase>(PlayerState);

		if (true == IsValid(FPSPlayerState) && false == FPSPlayerState->IsDead())
		{
			++AlivePlayerCount;
			LastAlivePlayer = FPSPlayerState;
		}
	}

	if (AlivePlayerCount <= 1)
	{
		// 플레이어가 한명 남아있다면 마지막 남은 플레이어가 우승자다.
		// FinishMatch(LastAlivePlayer); 
		// 함수 구현 아직 안함.
	}
}
