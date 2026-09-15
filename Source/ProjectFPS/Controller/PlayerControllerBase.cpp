// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/PlayerControllerBase.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputSubsystems.h"

APlayerControllerBase::APlayerControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlayerControllerBase::ChangeState(FName NewState)
{
	Super::ChangeState(NewState);

	// 엔진의 Pawn/관전자 전환이 끝난 뒤 입력을 맞춘다.
	RefreshInputMappingcontext();
}

void APlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// 로비의 UI 입력 모드 잔재 방빚 -> 게임 진입 시 게임 입력 모드로 명시
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}

	RefreshInputMappingcontext();
}

void APlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void APlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();

	RefreshInputMappingcontext();
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

void APlayerControllerBase::EnterDeathSpectating(const FVector& CameraaLocation, const FRotator& CameraRotation)
{
	if (false == HasAuthority())
	{
		return;
	}

	SetSpawnLocation(CameraaLocation);
	SetControlRotation(CameraRotation);

	// 서버 상태와 PlayerState의 관전자 플래그 설정
	StartSpectatingOnly();

	// 클라이언트의 컨트롤러 상태 변경
	ClientGotoState(NAME_Spectating);
}

void APlayerControllerBase::RefreshInputMappingcontext()
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
	//else if (GetStateName() == NAME_Spectating)
	//{
	//	//DesiredContext = _SpectatorMappingContext.Get();
	//}

	// 함수 포인터
	auto RemoveIfInactive = [&](UInputMappingContext* Context)
		{
			if (true == IsValid(Context) &&
				Context != DesiredContext && 
				Subsystem->HasMappingContext(Context))
			{
				Subsystem->RemoveMappingContext(Context);
			}
		};

	RemoveIfInactive(_PlayerMappingContext.Get());
	//RemoveIfInactive(_SpectatorMappingContext.Get());

	if (true == IsValid(DesiredContext) && false == Subsystem->HasMappingContext(DesiredContext))
	{
		Subsystem->AddMappingContext(DesiredContext, 0);
	}
}
