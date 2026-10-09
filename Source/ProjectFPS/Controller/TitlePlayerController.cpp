#include "Controller/TitlePlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "TimerManager.h"
#include "UI/Title/TitleScreenWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogTitlePlayerController, Log, All);

ATitlePlayerController::ATitlePlayerController()
{
	LobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/LobbyLevel.LobbyLevel")));

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bAutoManageActiveCameraTarget = false;
}

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTitlePlayerController, Error, TEXT("Cannot initialize the title menu without a valid world."));
		return;
	}

	bLeavingTitle = false;
	InitializeTitleCamera(*World);
	if (!InitializeTitleWidget())
	{
		return;
	}

	UE_LOG(LogTitlePlayerController, Display, TEXT("Title menu ready; lobby=%s"),
		*LobbyLevel.ToSoftObjectPath().ToString());
}

void ATitlePlayerController::InitializeTitleCamera(UWorld& World)
{
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* CameraActor = *It;
		if (IsValid(CameraActor) && CameraActor->ActorHasTag(TitleCameraTag))
		{
			SetViewTarget(CameraActor);
			return;
		}
	}

	UE_LOG(LogTitlePlayerController, Warning, TEXT("No actor tagged %s was found; using the current view target."),
		*TitleCameraTag.ToString());
}

bool ATitlePlayerController::InitializeTitleWidget()
{
	if (!IsValid(TitleWidgetClass.Get()))
	{
		UE_LOG(LogTitlePlayerController, Error,
			TEXT("TitleWidgetClass must reference a Widget Blueprint derived from TitleScreenWidget."));
		return false;
	}

	TitleWidget = CreateWidget<UTitleScreenWidget>(this, TitleWidgetClass);
	if (!IsValid(TitleWidget))
	{
		UE_LOG(LogTitlePlayerController, Error, TEXT("Failed to create the title widget from %s."),
			*GetNameSafe(TitleWidgetClass.Get()));
		return false;
	}

	TitleWidget->AddToViewport(TitleWidgetZOrder);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TitleWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	return true;
}

void ATitlePlayerController::StartFromTitle()
{
	if (bLeavingTitle || !IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTitlePlayerController, Error, TEXT("Cannot start title travel without a valid world."));
		return;
	}

	// A soft reference may be valid even when its level is not loaded yet.
	if (LobbyLevel.IsNull())
	{
		UE_LOG(LogTitlePlayerController, Error, TEXT("Cannot start title travel: LobbyLevel is not configured."));
		return;
	}

	PendingLobbyLevel = LobbyLevel;
	bLeavingTitle = true;

	if (IsValid(TitleWidget))
	{
		TitleWidget->SetIsEnabled(false);
	}

	PauseTitleSequences(*World);

	const float FadeDuration = FMath::Max(TitleFadeDuration, 0.0f);
	const float TravelDelay = FMath::Max(TitleTravelDelay, FadeDuration);

	if (IsValid(PlayerCameraManager))
	{
		if (FadeDuration > 0.0f)
		{
			PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeDuration, FLinearColor::Black, false, true);
		}
		else
		{
			PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
		}
	}

	UE_LOG(LogTitlePlayerController, Display, TEXT("Title Start clicked: transition to lobby %s"),
		*PendingLobbyLevel.ToSoftObjectPath().ToString());

	if (TravelDelay > 0.0f)
	{
		World->GetTimerManager().SetTimer(TravelTimer, this, &ThisClass::FinishTravel, TravelDelay, false);
	}
	else
	{
		FinishTravel();
	}
}

void ATitlePlayerController::PauseTitleSequences(UWorld& World)
{
	for (TActorIterator<ALevelSequenceActor> It(&World); It; ++It)
	{
		ALevelSequenceActor* SequenceActor = *It;
		if (!IsValid(SequenceActor))
		{
			continue;
		}

		ULevelSequencePlayer* SequencePlayer = SequenceActor->GetSequencePlayer();
		if (IsValid(SequencePlayer) && SequencePlayer->IsPlaying())
		{
			SequencePlayer->Pause();
		}
	}
}

void ATitlePlayerController::FinishTravel()
{
	if (!bLeavingTitle || !IsLocalController() || !IsValid(GetWorld()) || PendingLobbyLevel.IsNull())
	{
		return;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingLobbyLevel, true);
}

void ATitlePlayerController::QuitFromTitle()
{
	if (bLeavingTitle || !IsLocalController() || !IsValid(GetWorld()))
	{
		return;
	}

	bLeavingTitle = true;
	if (IsValid(TitleWidget))
	{
		TitleWidget->SetIsEnabled(false);
	}

	UE_LOG(LogTitlePlayerController, Display, TEXT("Title Quit clicked: quit game"));
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ATitlePlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	bLeavingTitle = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TravelTimer);
	}
	TravelTimer.Invalidate();
	PendingLobbyLevel.Reset();

	if (IsValid(TitleWidget))
	{
		TitleWidget->RemoveFromParent();
	}
	TitleWidget = nullptr;

	Super::EndPlay(Reason);
}

