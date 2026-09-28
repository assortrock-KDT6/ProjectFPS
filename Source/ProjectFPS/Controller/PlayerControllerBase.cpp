// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/PlayerControllerBase.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"

APlayerControllerBase::APlayerControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlayerControllerBase::ChangeState(FName NewState)
{
	Super::ChangeState(NewState);

	// 엔진의 Pawn/관전자 전환이 끝난 뒤 입력을 맞춘다.
	RefreshInputMappingContext();
}

void APlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// 로비의 UI 입력 모드 잔재 방비 -> 게임 진입 시 게임 입력 모드로 명시
	if (true == IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}

	RefreshInputMappingContext();
}

void APlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RefreshInputMappingContext();
}

void APlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();

	RefreshInputMappingContext();
}

void APlayerControllerBase::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	RefreshInputMappingContext();
}

void APlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();

	if (nullptr != LocalPlayer)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem< UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

		if (true == IsValid(Subsystem))
		{
			if (true == IsValid(_PlayerMappingContext))
			{
				Subsystem->RemoveMappingContext(_PlayerMappingContext.Get());
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void APlayerControllerBase::EnterDeathSpectating(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	if (false == HasAuthority())
	{
		return;
	}

	SetSpawnLocation(CameraLocation);

	SetControlRotation(CameraRotation);

	// 참가자 자격은 유지하고 리스폰을 기다리는 동안만 관전한다.
	if (true == IsValid(PlayerState))
	{
		PlayerState->SetIsSpectator(false);

		PlayerState->SetIsOnlyASpectator(false);
	}
	ChangeState(NAME_Spectating);

	bPlayerIsWaiting = true;

	ClientEnterDeathSpectating(CameraLocation, CameraRotation);
}

void APlayerControllerBase::ClientEnterDeathSpectating_Implementation(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	SetSpawnLocation(CameraLocation);

	SetControlRotation(CameraRotation);

	ChangeState(NAME_Spectating);

	bPlayerIsWaiting = true;
}

void APlayerControllerBase::RefreshInputMappingContext()
{
	if (false == IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (false == IsValid(LocalPlayer))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (false == IsValid(Subsystem))
	{
		return;
	}

	UInputMappingContext* DesiredContext = nullptr;

	if (NAME_Playing == GetStateName() && true == IsValid(GetPawn()))
	{
		DesiredContext = _PlayerMappingContext.Get();
	}
	if (IsValid(_PlayerMappingContext) && _PlayerMappingContext.Get() != DesiredContext
		&& Subsystem->HasMappingContext(_PlayerMappingContext.Get()))
	{
		Subsystem->RemoveMappingContext(_PlayerMappingContext.Get());
	}

	if (true == IsValid(DesiredContext) && false == Subsystem->HasMappingContext(DesiredContext))
	{
		Subsystem->AddMappingContext(DesiredContext, 0);
	}
}
