// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/PlayerStateBase.h"
#include "Component/Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameMode/FPSLobbyGameState.h"
#include "GameMode/PlayerMatchStats.h"
#include "Engine/World.h"

APlayerStateBase::APlayerStateBase()
{
	_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void APlayerStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerStateBase, _IsDead);
	DOREPLIFETIME(APlayerStateBase, _MatchStats);
}

bool APlayerStateBase::IsDead() const
{
	return _IsDead;
}

void APlayerStateBase::SetDead(bool IsDead)
{
	if (false == HasAuthority() || _IsDead == IsDead)
	{
		return;
	}
	_IsDead = IsDead;
	ForceNetUpdate();
	OnRep_IsDead();
}

void APlayerStateBase::AddKillScore()
{
	if (false == HasAuthority())
	{
		return;
	}
	++_MatchStats._KillScore;

	SetScore(static_cast<float>(_MatchStats._KillScore));

	ForceNetUpdate();

	OnRep_MatchStats();
}

void APlayerStateBase::AddDeathScore()
{
	if (false == HasAuthority())
	{
		return;
	}
	++_MatchStats._DeathScore;
	ForceNetUpdate();
	OnRep_MatchStats();
}

void APlayerStateBase::ResetMatchStats()
{
	if (false == HasAuthority())
	{
		return;
	}
	_MatchStats = FPlayerMatchStats();
	SetScore(0.f);
	ForceNetUpdate();
	OnRep_MatchStats();
}

void APlayerStateBase::OnRep_IsDead()
{
	_OnDeathStateChanged.Broadcast(_IsDead);
}

void APlayerStateBase::OnRep_MatchStats()
{
	_OnMatchStatsChanged.Broadcast();
}
	

void APlayerStateBase::SetPlayerName(const FString& Name)
{
    Super::SetPlayerName(Name);
    if (auto* Lobby = GetWorld() ? GetWorld()->GetGameState<AFPSLobbyGameState>() : nullptr) { Lobby->NotifyRosterChanged(); }
}
void APlayerStateBase::OnRep_PlayerName()
{
    Super::OnRep_PlayerName();
    if (auto* Lobby = GetWorld() ? GetWorld()->GetGameState<AFPSLobbyGameState>() : nullptr) { Lobby->NotifyRosterChanged(); }
}
void APlayerStateBase::OnRep_bIsInactive()
{
    Super::OnRep_bIsInactive();
    if (auto* Lobby = GetWorld() ? GetWorld()->GetGameState<AFPSLobbyGameState>() : nullptr) { Lobby->NotifyRosterChanged(); }
}
