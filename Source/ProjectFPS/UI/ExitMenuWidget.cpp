#include "UI/ExitMenuWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"

void UExitMenuWidget::NativeConstruct()
{
	if (auto* Sessions = GetSessionSubsystem())
	{
		Sessions->_OnExitCleanupCompleted.AddUniqueDynamic(this, &ThisClass::HandleExitCleanupCompleted);
	}
	Super::NativeConstruct();
}

void UExitMenuWidget::NativeDestruct()
{
	if (auto* Sessions = GetSessionSubsystem())
	{
		Sessions->_OnExitCleanupCompleted.RemoveDynamic(this, &ThisClass::HandleExitCleanupCompleted);
	}

	Super::NativeDestruct();
}

UFPSOnlineSessionSubsystem* UExitMenuWidget::GetSessionSubsystem() const
{
	UGameInstance* Instance = GetGameInstance();

	return Instance ? Instance->GetSubsystem<UFPSOnlineSessionSubsystem>() : nullptr;
}

FExitMenuStatus UExitMenuWidget::GetExitStatus() const
{
	FExitMenuStatus Status;
	const UFPSOnlineSessionSubsystem* Sessions = GetSessionSubsystem();

	if (!IsValid(Sessions)) 
	{
		return Status;
	}

	Status.Busy = Sessions->IsBusy() || Sessions->IsAutoMatchInProgress();

	Status.ExitInProgress = Sessions->IsExitInProgress();

	Status.Host = Sessions->GetConnectionState() == EFPSOnlineConnectionState::Hosting
		|| (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer);

	const FString LobbyPath = Sessions->GetLobbyMapPath();

	const FString CurrentPath = GetWorld() ? UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName()) : FString();

	Status.CanReturnToLobby = !LobbyPath.IsEmpty() && (CurrentPath != LobbyPath
		|| Sessions->GetConnectionState() != EFPSOnlineConnectionState::None);

	return Status;
}

bool UExitMenuWidget::TryExit(bool QuitApplication)
{
	const APlayerController* Player = GetOwningPlayer();

	UFPSOnlineSessionSubsystem* Sessions = GetSessionSubsystem();

	const FExitMenuStatus Status = GetExitStatus();

	if (!IsValid(Player) || !Player->IsLocalController() || !IsValid(Sessions)
		|| Status.Busy || Status.ExitInProgress || (!QuitApplication && !Status.CanReturnToLobby))
	{
		return false;
	}

	return Sessions->RequestExit(QuitApplication);
}
