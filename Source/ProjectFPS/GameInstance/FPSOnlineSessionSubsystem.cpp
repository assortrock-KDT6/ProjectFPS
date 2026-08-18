// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Engine/World.h"
#include "Controller/PlayerControllerBase.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "Common/GameDatas.h"

DEFINE_LOG_CATEGORY(LogFPSOnlineSession);

#pragma region DEBUG_FUNCTION
const TCHAR* SessionStateToString(const EFPSOnlineSessionState& State)
{
	switch (State)
	{
	case  EFPSOnlineSessionState::Idle:
		return TEXT("Idle");
	case EFPSOnlineSessionState::Destroying:
		return TEXT("Destroying");
	case EFPSOnlineSessionState::Creating:
		return TEXT("Creating");
	case EFPSOnlineSessionState::Finding:
		return TEXT("Finding");
	case EFPSOnlineSessionState::Joining:
		return TEXT("Joining");
	case EFPSOnlineSessionState::Traveling:
		return TEXT("Traveling");
	default:
		return TEXT("Unknown");
	}
}

FString JoinResultToError(EOnJoinSessionCompleteResult::Type Result)
{
	switch (Result)
	{
	case EOnJoinSessionCompleteResult::SessionIsFull:
		return TEXT("Session is full.");
	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		return TEXT("Session does not exist.");
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
		return TEXT("Could not retrieve the session Address.");
	case EOnJoinSessionCompleteResult::AlreadyInSession:
		return TEXT("Alreadyin this session.");
	default:
		return TEXT("Unkown join session error.");
	}
}

bool IsNullSubsystem(const UWorld* World)
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	return Subsystem && Subsystem->GetSubsystemName() == FName(TEXT("NULL"));
}
#pragma endregion


void UFPSOnlineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	_SessionState = EFPSOnlineSessionState::Idle;
	RefreshSessionInterface();

	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());

#if !UE_BUILD_SHIPPING
	if (Subsystem)
	{
		const FString SubsystemName = Subsystem->GetSubsystemName().ToString();
		UE_LOG(LogFPSOnlineSession, Log, TEXT("Initialize: OSS = %s SessionInterface = %s"), *SubsystemName, _SessionInterface.IsValid() ? TEXT("Vaild") : TEXT("InValid"));
	}
#endif // !UE_BUILD_SHIPPING
}

void UFPSOnlineSessionSubsystem::Deinitialize()
{
	ClearAllOnlineDelegates();
	_SessionSearch.Reset();
	_SessionInterface.Reset();
	ResetPendingCreate();
	_SessionState = EFPSOnlineSessionState::Idle;

	Super::Deinitialize();
}

bool UFPSOnlineSessionSubsystem::CreateSession(int32 PublicConnections, const FString& GameMapPath)
{
	FString Error;
	if (false == RequireIdle(TEXT("CreateSession"), Error))
	{
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (PublicConnections <= 0)
	{
		Error = TEXT("PublicConnections must be greater than zero.");
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	_PendingGameMapPath = GameMapPath;
	_PendingGameMapPath.TrimStartAndEndInline();
	_PendingPublicConnections = PublicConnections;

	if (_PendingGameMapPath.IsEmpty() || nullptr == GetWorld())
	{
		Error = TEXT("A valid World and game map path required.");
		ResetPendingCreate();
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		Error = TEXT("Online Session interace is unavaliable.");
		ResetPendingCreate();
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		_CreateAfterDestoy = true;
		return StartDestroySession();
	}
	return StartCreateSession();
}

bool UFPSOnlineSessionSubsystem::FindSessions(int32 MaxResults)
{
	FString Error;
	TArray<FFPSOnlineSessionInfo> EmptySessions;

	if (false == RequireIdle(TEXT("FindSessions"), Error))
	{
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, Error);
		return false;
	}

	if (MaxResults <= 0 || !RefreshSessionInterface())
	{
		if (MaxResults <= 0)
		{
			Error = TEXT("MaxResults must be greater than zero.");
		}
		else
		{
			Error = TEXT("Online Session interface is unavailable.");
		}
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, Error);
		return false;
	}

	_SessionSearch = MakeShared<FOnlineSessionSearch>();
	_SessionSearch->MaxSearchResults = MaxResults;
	_SessionSearch->bIsLanQuery = IsNullSubsystem(GetWorld());
	_SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	if (_FindSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(_FindSessionCompleteHandle);
	}

	_FindSessionCompleteHandle = _SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionComplete));

	SetSessionState(EFPSOnlineSessionState::Finding);

	if (false == _SessionInterface->FindSessions(0, _SessionSearch.ToSharedRef()))
	{
		_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(_FindSessionCompleteHandle);
		_FindSessionCompleteHandle.Reset();
		SetSessionState(EFPSOnlineSessionState::Idle);
		Error = TEXT("FindSessions request could not be started.");
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, Error);
		return false;
	}
	return true;
}

bool UFPSOnlineSessionSubsystem::JoinSessionByIndex(int32 ResultIndex)
{
	FString Error;
	if (false == RequireIdle(TEXT("JoinSession"), Error))
	{
		_OnJoinSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (false == _SessionSearch.IsValid() ||
		false == _SessionSearch->SearchResults.IsValidIndex(ResultIndex) ||
		false == _SessionSearch->SearchResults[ResultIndex].IsValid())
	{
		Error = TEXT("The selected session result is invalid");
		_OnJoinSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		Error = TEXT("Online Session interface is unavailable.");
		_OnJoinSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (_JoinSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(_JoinSessionCompleteHandle);
	}

	_JoinSessionCompleteHandle = _SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	SetSessionState(EFPSOnlineSessionState::Joining);

	if (false == _SessionInterface->JoinSession(0, NAME_GameSession, _SessionSearch->SearchResults[ResultIndex]))
	{
		_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(_JoinSessionCompleteHandle);
		_JoinSessionCompleteHandle.Reset();
		SetSessionState(EFPSOnlineSessionState::Idle);

		Error = TEXT("JoinSession request could not be started.");
		_OnJoinSessionCompleted.Broadcast(false, Error);
		return false;
	}

	return true;
}

bool UFPSOnlineSessionSubsystem::DestroySession()
{
	FString Error;
	if (false == RequireIdle(TEXT("DestroySession"), Error))
	{
		_OnDestroySessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		Error = TEXT("Online Session interface is unavailable.");
		_OnDestroySessionCompleted.Broadcast(false, Error);
		return false;
	}

	_CreateAfterDestoy = false;

	if (nullptr == _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		_OnDestroySessionCompleted.Broadcast(true, FString());
		return true;
	}
	return StartDestroySession();
}

EFPSOnlineSessionState UFPSOnlineSessionSubsystem::GetSessionState() const
{
	return _SessionState;
}

bool UFPSOnlineSessionSubsystem::IsBusy()
{
	return EFPSOnlineSessionState::Idle != _SessionState;
}

bool UFPSOnlineSessionSubsystem::RefreshSessionInterface()
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (nullptr == Subsystem)
	{
		return false;
	}

	_SessionInterface = Subsystem->GetSessionInterface();
	return _SessionInterface.IsValid();
}

bool UFPSOnlineSessionSubsystem::RequireIdle(const TCHAR* Operation, FString& OutError) const
{
	if (EFPSOnlineSessionState::Idle == _SessionState)
	{
		return true;
	}

	OutError = FString::Printf(TEXT("%srejected: current state is %s."), Operation, SessionStateToString(_SessionState));
	return false;
}

bool UFPSOnlineSessionSubsystem::StartCreateSession()
{
	if (false == _SessionInterface.IsValid())
	{
		const FString Error = TEXT("Session interface before CreateSession.");
		ResetPendingCreate();
		SetSessionState(EFPSOnlineSessionState::Idle);
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}


	// TODO : Config Struct 
	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = _PendingPublicConnections;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bIsLANMatch = IsNullSubsystem(GetWorld());
	Settings.BuildUniqueId = 1;
	Settings.Set(SETTING_MAPNAME, _PendingGameMapPath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (_CreateSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
	}

	_CreateSessionCompleteHandle = _SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));
	SetSessionState(EFPSOnlineSessionState::Creating);

	if (false == _SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
		_CreateSessionCompleteHandle.Reset();
		ResetPendingCreate();
		SetSessionState(EFPSOnlineSessionState::Idle);

		const FString Error = TEXT("CreateSession request could not be started.");
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}
	return true;
}

bool UFPSOnlineSessionSubsystem::StartDestroySession()
{
	if (false == _SessionInterface.IsValid())
	{
		const bool PendingCreate = _CreateAfterDestoy;
		ResetPendingCreate();
		SetSessionState(EFPSOnlineSessionState::Idle);

		const FString Error = TEXT("Session interface before DestroySession");
		_OnDestroySessionCompleted.Broadcast(false, Error);
		if (true == PendingCreate)
		{
			_OnCreateSessionCompleted.Broadcast(false, Error);
		}
		return false;
	}

	if (_DestroySessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
	}

	_DestroySessionCompleteHandle = _SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));

	SetSessionState(EFPSOnlineSessionState::Destroying);
	
	if (false == _SessionInterface->DestroySession(NAME_GameSession))
	{
		_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
		_DestroySessionCompleteHandle.Reset();

		const bool PendingCreate = _CreateAfterDestoy;
		ResetPendingCreate();
		SetSessionState(EFPSOnlineSessionState::Idle);

		const FString Error = TEXT("DestroySession request could not be started.");

		_OnDestroySessionCompleted.Broadcast(false, Error);
		if (true == PendingCreate)
		{
			_OnCreateSessionCompleted.Broadcast(false, Error);
		}
		return false;
	}
	return true;
}

void UFPSOnlineSessionSubsystem::SetSessionState(EFPSOnlineSessionState NewState)
{
	if (_SessionState == NewState)
	{
		return;
	}

	_SessionState = NewState;
	_OnSessionStateChanged.Broadcast(NewState);
}

void UFPSOnlineSessionSubsystem::ClearAllOnlineDelegates()
{
	if (true == _SessionInterface.IsValid())
	{
		if (true == _CreateSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
		}
		if (true == _FindSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(_FindSessionCompleteHandle);
		}
		if (true == _JoinSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(_JoinSessionCompleteHandle);
		}
		if (true == _DestroySessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
		}
	}

	_CreateSessionCompleteHandle.Reset();
	_FindSessionCompleteHandle.Reset();
	_JoinSessionCompleteHandle.Reset();
	_DestroySessionCompleteHandle.Reset();
}

void UFPSOnlineSessionSubsystem::ResetPendingCreate()
{
	_PendingGameMapPath.Reset();
	_PendingPublicConnections = 0;
	_CreateAfterDestoy = false;
}

void UFPSOnlineSessionSubsystem::TravelHostToGame()
{
	UWorld* World = GetWorld();
	if (nullptr == World)
	{
		SetSessionState(EFPSOnlineSessionState::Idle);
		UE_LOG(LogFPSOnlineSession, Error, TEXT("ServerTravel failed: no World."));
		return;
	}

	FString TravelURL = _PendingGameMapPath;
	if (false == TravelURL.Contains(TEXT("?listen"), ESearchCase::IgnoreCase))
	{
		TravelURL += TEXT("?listen");
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogFPSOnlineSession, Log, TEXT("ServerTravel: %s"), *TravelURL);
#endif // !UE_BUILD_SHIPPING

	if (false == World->ServerTravel(TravelURL))
	{
		SetSessionState(EFPSOnlineSessionState::Idle);
		UE_LOG(LogFPSOnlineSession, Error, TEXT("ServerTravel was rejected: %s"), *TravelURL);
	}
}

void UFPSOnlineSessionSubsystem::TravelClientToSession(FName SessionName)
{
	FString ConnectString;
	if (false == _SessionInterface.IsValid() ||
		false == _SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		SetSessionState(EFPSOnlineSessionState::Idle);
		_OnJoinSessionCompleted.Broadcast(false, TEXT("Could not resolve the session"));
		return;
	}

	UWorld* World = GetWorld();
	if (nullptr != World)
	{
		APlayerControllerBase* PlayerController = Cast<APlayerControllerBase>(World->GetFirstPlayerController());
		if (false == IsValid(PlayerController) || false == PlayerController->IsLocalController())
		{
			SetSessionState(EFPSOnlineSessionState::Idle);
			_OnJoinSessionCompleted.Broadcast(false, TEXT("Local PlayerController is unavailable."));
			return;
		}

		SetSessionState(EFPSOnlineSessionState::Traveling);
		_OnJoinSessionCompleted.Broadcast(true, FString());
		PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	}
	else
	{
		SetSessionState(EFPSOnlineSessionState::Idle);
		_OnJoinSessionCompleted.Broadcast(false, TEXT("World is unvailable."));
		return;
	}
}

void UFPSOnlineSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool WasSuccessful)
{
	if (true == _SessionInterface.IsValid() && true == _CreateSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
	}
	_CreateSessionCompleteHandle.Reset();

	if (false == WasSuccessful)
	{
		ResetPendingCreate();
		SetSessionState(EFPSOnlineSessionState::Idle);
		_OnCreateSessionCompleted.Broadcast(false, TEXT("Online subsystem failed to create the session."));
		return;
	}

	SetSessionState(EFPSOnlineSessionState::Traveling);
	_OnCreateSessionCompleted.Broadcast(true, FString());
	TravelHostToGame();
	ResetPendingCreate();
}

void UFPSOnlineSessionSubsystem::HandleFindSessionComplete(bool WasSuccessful)
{
	if (true == _SessionInterface.IsValid() && true == _FindSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(_FindSessionCompleteHandle);
	}
	_FindSessionCompleteHandle.Reset();
	SetSessionState(EFPSOnlineSessionState::Idle);

	TArray<FFPSOnlineSessionInfo> Sessions;
	if (false == WasSuccessful || false == _SessionSearch.IsValid())
	{
		_OnFindSessionCompleted.Broadcast(false, Sessions, TEXT("Online subsystem failed to find sessions."));
		return;
	}

	Sessions.Reserve(_SessionSearch->SearchResults.Num());
	for (int32 Index = 0; Index < _SessionSearch->SearchResults.Num(); Index++)
	{
		const FOnlineSessionSearchResult& Result = _SessionSearch->SearchResults[Index];
		
		FFPSOnlineSessionInfo& Information = Sessions.AddDefaulted_GetRef();
		Information._ResultIndex = Index;
		Information._SessionOwnerName = Result.Session.OwningUserName;
		Result.Session.SessionSettings.Get(SETTING_MAPNAME, Information._MapName);
		Information._PingInMs = Result.PingInMs;
		Information._MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Information._CurrentPlayers = FMath::Clamp(Information._MaxPlayers - Result.Session.NumOpenPublicConnections, 0, Information._MaxPlayers);
		Information._IsLan = Result.Session.SessionSettings.bIsLANMatch;
	}

	_OnFindSessionCompleted.Broadcast(true, Sessions, FString());
}

void UFPSOnlineSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (true == _SessionInterface.IsValid() && true == _JoinSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(_JoinSessionCompleteHandle);
	}
	_JoinSessionCompleteHandle.Reset();

#if !UE_BUILD_SHIPPING
	UE_LOG(LogFPSOnlineSession, Log, TEXT("JoinSession result: %s"), LexToString(Result));
#endif // !UE_BUILD_SHIPPING

	if (EOnJoinSessionCompleteResult::Success != Result)
	{
		SetSessionState(EFPSOnlineSessionState::Idle);
		_OnJoinSessionCompleted.Broadcast(false, JoinResultToError(Result));
		return;
	}

	TravelClientToSession(SessionName);
}

void UFPSOnlineSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool WasSuccessful)
{
	if (true == _SessionInterface.IsValid() && true == _DestroySessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
	}
	_DestroySessionCompleteHandle.Reset();

	const bool ShouldCreate = _CreateAfterDestoy;
	_CreateAfterDestoy = false;

	if (false == WasSuccessful)
	{
		const FString Error = TEXT("Online subsystem failed to destroy the session.");
		ResetPendingCreate();
		SetSessionState(EFPSOnlineSessionState::Idle);
		_OnDestroySessionCompleted.Broadcast(false, Error);

		if (true == ShouldCreate)
		{
			_OnCreateSessionCompleted.Broadcast(false, Error);
		}
		return;
	}

	_OnDestroySessionCompleted.Broadcast(true, FString());
	if (true == ShouldCreate)
	{
		StartCreateSession();
		return;
	}

	SetSessionState(EFPSOnlineSessionState::Idle);
}
