#include "GameMode/FPSLobbyGameState.h"
#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

void AFPSLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFPSLobbyGameState, _SelectedMap);
}

void AFPSLobbyGameState::NotifyRosterChanged()
{
	_OnLobbyChanged.Broadcast();
}

const FFPSPlayableMap& AFPSLobbyGameState::GetSelectedMap() const
{
	return _SelectedMap;
}

void AFPSLobbyGameState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && GetGameInstance())
	{
		if (const auto* Sessions = GetGameInstance()->GetSubsystem<UFPSOnlineSessionSubsystem>())
		{
			SetSelectedMap(Sessions->GetSelectedGameMap());
		}
	}
}

void AFPSLobbyGameState::SetSelectedMap(const FFPSPlayableMap& Map)
{
	if (HasAuthority())
	{
		_SelectedMap = Map;

		ForceNetUpdate();

        OnRep_SelectedMap();
	}
}

void AFPSLobbyGameState::OnRep_SelectedMap() 
{ 
	_OnLobbyChanged.Broadcast(); 
}

void AFPSLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
    Super::AddPlayerState(PlayerState);
    NotifyRosterChanged();
}

void AFPSLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);
    NotifyRosterChanged();
}
