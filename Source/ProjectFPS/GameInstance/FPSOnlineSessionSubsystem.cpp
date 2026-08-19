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
		return TEXT("Already in this session.");
	default:
		return TEXT("Unknown join session error.");
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

	_OperationState = EFPSOnlineOperationState::Idle;
	RefreshSessionInterface();

	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());

#if !UE_BUILD_SHIPPING
	if (Subsystem)
	{
		const FString SubsystemName = Subsystem->GetSubsystemName().ToString();
		UE_LOG(LogFPSOnlineSession, Log, TEXT("Initialize: OSS = %s SessionInterface = %s"), *SubsystemName, _SessionInterface.IsValid() ? TEXT("Valid") : TEXT("Invalid"));
	}
#endif // !UE_BUILD_SHIPPING
}

bool UFPSOnlineSessionSubsystem::RefreshSessionInterface()
{
	_SessionInterface.Reset();

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (nullptr == Subsystem)
	{
		return false;
	}

	_SessionInterface = Subsystem->GetSessionInterface();
	return _SessionInterface.IsValid();
}

void UFPSOnlineSessionSubsystem::Deinitialize()
{
	ClearAllOnlineDelegates();
	_SessionSearch.Reset();
	_SessionInterface.Reset();
	ResetPendingCreate();

	_PendingJoinError.Reset();
	_DestroyIntent = EFPSSessionDestroyIntent::None;
	_OperationState = EFPSOnlineOperationState::Idle;
	_ConnectionState = EFPSOnlineConnectionState::None;

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

	if (EFPSOnlineConnectionState::Joined == _ConnectionState)
	{
		Error = TEXT("Leave the current session before creating a new session.");
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (PublicConnections <= 0)
	{
		Error = TEXT("PublicConnections must be greater than zero.");
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	FString NormalizedMapPath = GameMapPath;
	NormalizedMapPath.TrimStartAndEndInline();

	if (false == CanServerTravel(NormalizedMapPath, Error))
	{
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	_PendingGameMapPath = MoveTemp(NormalizedMapPath);
	_PendingPublicConnections = PublicConnections;

	if (false == RefreshSessionInterface())
	{
		Error = TEXT("Online Session interace is unavaliable.");
		ResetPendingCreate();
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		return StartDestroySession(EFPSSessionDestroyIntent::Recreate);
	}
	return StartCreateSession();
}

bool UFPSOnlineSessionSubsystem::StartCreateSession()
{
	if (false == _SessionInterface.IsValid())
	{
		const FString Error = TEXT("Session interface before CreateSession.");
		ResetPendingCreate();
		SetOperationState(EFPSOnlineOperationState::Idle);
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

	SetOperationState(EFPSOnlineOperationState::Creating);

	if (false == _SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
		_CreateSessionCompleteHandle.Reset();
		ResetPendingCreate();
		SetOperationState(EFPSOnlineOperationState::Idle);

		const FString Error = TEXT("CreateSession request could not be started.");
		_OnCreateSessionCompleted.Broadcast(false, Error);
		return false;
	}
	return true;
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
		SetOperationState(EFPSOnlineOperationState::Idle);
		_OnCreateSessionCompleted.Broadcast(false, TEXT("Online subsystem failed to create the session."));
		return;
	}

	SetConnectionState(EFPSOnlineConnectionState::Hosting);
	SetOperationState(EFPSOnlineOperationState::Idle);

	_OnCreateSessionCompleted.Broadcast(true, FString());

	FString TravelError;

	if (false == TravelHostToGame(TravelError))
	{
		_OnTravelFailed.Broadcast(TravelError);
	}
	ResetPendingCreate();
}

bool UFPSOnlineSessionSubsystem::CanServerTravel(const FString& MapPath, FString& OutError) const
{
	UWorld* World = GetWorld();

	if (false == IsValid(World))
	{
		OutError = TEXT("World is unavailable.");
		return false;
	}

	if (NM_Client == World->GetNetMode())
	{
		OutError = TEXT("Client cannot call ServerTravel.");
		return false;
	}

	if (true == MapPath.IsEmpty())
	{
		OutError = TEXT("MapPath is Empty.");
		return false;
	}

	if (false == FPackageName::IsValidLongPackageName(MapPath))
	{
		OutError = FString::Printf(TEXT("Invalid Map Package path: %s"), *MapPath);
		return false;
	}

	if (false == FPackageName::DoesPackageExist(MapPath))
	{
		OutError = FString::Printf(TEXT("Map Package does not exist: %s"), *MapPath);
		return false;
	}

	return true;
}

bool UFPSOnlineSessionSubsystem::TravelHostToGame(FString& OutError)
{
	UWorld* World = GetWorld();
	if (nullptr == World)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
		OutError = TEXT("World is unavailable.");
		return false;
	}

	FString TravelURL = _PendingGameMapPath;
	if (false == TravelURL.Contains(TEXT("?listen"), ESearchCase::IgnoreCase))
	{
		TravelURL += TEXT("?listen");
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogFPSOnlineSession, Log, TEXT("ServerTravel: %s"), *TravelURL);
#endif // !UE_BUILD_SHIPPING
	
	if (false == CanServerTravel(_PendingGameMapPath, OutError))
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
#if !UE_BUILD_SHIPPING
		UE_LOG(LogFPSOnlineSession, Error, TEXT("ServerTravel was rejected: %s"), *TravelURL);
		UE_LOG(LogFPSOnlineSession, Error, TEXT("%s"), *OutError);
#endif // !UE_BUILD_SHIPPING
		return false;
	}

	SetOperationState(EFPSOnlineOperationState::Idle);
	if (false == World->ServerTravel(TravelURL))
	{
		OutError = FString::Printf(TEXT("ServerTravel rejected URL: %s"), *TravelURL);
		return false;
	}
	return true;
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

	SetOperationState(EFPSOnlineOperationState::Finding);

	if (false == _SessionInterface->FindSessions(0, _SessionSearch.ToSharedRef()))
	{
		_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(_FindSessionCompleteHandle);
		_FindSessionCompleteHandle.Reset();
		SetOperationState(EFPSOnlineOperationState::Idle);
		Error = TEXT("FindSessions request could not be started.");
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, Error);
		return false;
	}
	return true;
}

void UFPSOnlineSessionSubsystem::HandleFindSessionComplete(bool WasSuccessful)
{
	if (true == _SessionInterface.IsValid() && true == _FindSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(_FindSessionCompleteHandle);
	}
	_FindSessionCompleteHandle.Reset();
	SetOperationState(EFPSOnlineOperationState::Idle);

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


bool UFPSOnlineSessionSubsystem::JoinSessionByIndex(int32 ResultIndex)
{
	FString Error;

	if (false == RequireIdle(TEXT("JoinSession"), Error))
	{
		_OnJoinSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (EFPSOnlineConnectionState::None != _ConnectionState)
	{
		Error = TEXT("JoinSession requires a disconnected state.");
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

	SetOperationState(EFPSOnlineOperationState::Joining);

	if (false == _SessionInterface->JoinSession(0, NAME_GameSession, _SessionSearch->SearchResults[ResultIndex]))
	{
		_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(_JoinSessionCompleteHandle);
		_JoinSessionCompleteHandle.Reset();
		SetOperationState(EFPSOnlineOperationState::Idle);

		Error = TEXT("JoinSession request could not be started.");
		_OnJoinSessionCompleted.Broadcast(false, Error);
		return false;
	}

	return true;
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
		SetOperationState(EFPSOnlineOperationState::Idle);
		_OnJoinSessionCompleted.Broadcast(false, JoinResultToError(Result));
		return;
	}
	TravelClientToSession(SessionName);
}

void UFPSOnlineSessionSubsystem::TravelClientToSession(FName SessionName)
{
	FString ConnectString;

	if (false == _SessionInterface.IsValid() ||
		false == _SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		RollbackJoinedSession(TEXT("Could not resolve the session address"));
		return;
	}

	UWorld* World = GetWorld();
	if (nullptr != World)
	{
		APlayerControllerBase* PlayerController = Cast<APlayerControllerBase>(World->GetFirstPlayerController());
		if (false == IsValid(PlayerController) || false == PlayerController->IsLocalController())
		{
			RollbackJoinedSession(TEXT("Local PlayerController is unavailable."));
			return;
		}

		PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);

		SetConnectionState(EFPSOnlineConnectionState::Joined);
		SetOperationState(EFPSOnlineOperationState::Idle);

		_OnJoinSessionCompleted.Broadcast(true, FString());
	}
	else
	{
		RollbackJoinedSession(TEXT("World is unavailable."));
	}
}

void UFPSOnlineSessionSubsystem::RollbackJoinedSession(const FString& Error)
{
	_PendingJoinError = Error;

	if (true == _SessionInterface.IsValid() && _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		StartDestroySession(EFPSSessionDestroyIntent::JoinRollback);
		return;
	}

	SetConnectionState(EFPSOnlineConnectionState::None);
	SetOperationState(EFPSOnlineOperationState::Idle);

	_OnJoinSessionCompleted.Broadcast(false, _PendingJoinError);
	_PendingJoinError.Reset();
}

bool UFPSOnlineSessionSubsystem::LeaveSession()
{
	FString Error;
	if (false == RequireIdle(TEXT("LeaveSession"), Error))
	{
		_OnLeaveSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (EFPSOnlineConnectionState::Hosting == _ConnectionState)
	{
		Error = TEXT("A host must use DestroySession instead of LeaveSession.");
		_OnLeaveSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		Error = TEXT("Online Session interface is unavailable.");
		_OnLeaveSessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (nullptr == _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SetConnectionState(EFPSOnlineConnectionState::None);
		_OnLeaveSessionCompleted.Broadcast(true, FString());
		return true;
	}

	return StartDestroySession(EFPSSessionDestroyIntent::Leave);
}

bool UFPSOnlineSessionSubsystem::DestroySession()
{
	FString Error;
	if (false == RequireIdle(TEXT("DestroySession"), Error))
	{
		_OnDestroySessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (EFPSOnlineConnectionState::Joined == _ConnectionState)
	{
		Error = TEXT("A joined client must use LeaveSession.");
		_OnDestroySessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		Error = TEXT("Online Session interface is unavailable.");
		_OnDestroySessionCompleted.Broadcast(false, Error);
		return false;
	}

	if (nullptr == _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SetConnectionState(EFPSOnlineConnectionState::None);
		_OnDestroySessionCompleted.Broadcast(true, FString());
		return true;
	}
	return StartDestroySession(EFPSSessionDestroyIntent::Destroy);
}

void UFPSOnlineSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool WasSuccessful)
{
	if (true == _SessionInterface.IsValid() && true == _DestroySessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
	}
	_DestroySessionCompleteHandle.Reset();

	const EFPSSessionDestroyIntent CompletedIntent = _DestroyIntent;
	_DestroyIntent = EFPSSessionDestroyIntent::None;

	if (false == WasSuccessful)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
		const FString Error = TEXT("Online subsystem failed to destroy the session.");

		BroadcastDestroyFailure(CompletedIntent, Error);
		return;
	}

	SetConnectionState(EFPSOnlineConnectionState::None);

	if (EFPSSessionDestroyIntent::Recreate == CompletedIntent)
	{
		StartCreateSession();
	}
	else if (EFPSSessionDestroyIntent::Leave == CompletedIntent)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
		_OnLeaveSessionCompleted.Broadcast(true, FString());
	}
	else if (EFPSSessionDestroyIntent::Destroy == CompletedIntent)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
		_OnDestroySessionCompleted.Broadcast(true, FString());
	}
	else if (EFPSSessionDestroyIntent::JoinRollback == CompletedIntent)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);

		FString JoinError = MoveTemp(_PendingJoinError);
		_PendingJoinError.Reset();

		if (true == JoinError.IsEmpty())
		{
			JoinError = TEXT("Joined session was rolled back before travel.");
		}

		_OnJoinSessionCompleted.Broadcast(false, JoinError);
	}
	else
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
	}
}

bool UFPSOnlineSessionSubsystem::StartDestroySession(EFPSSessionDestroyIntent Intent)
{
	if (false == _SessionInterface.IsValid())
	{
		const FString Error = TEXT("Online session interface is unavailable.");
		SetOperationState(EFPSOnlineOperationState::Idle);
		BroadcastDestroyFailure(Intent, Error);
		return false;
	}

	_DestroyIntent = Intent;

	if (_DestroySessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
	}

	_DestroySessionCompleteHandle = _SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));

	SetOperationState(EFPSOnlineOperationState::Destroying);

	if (false == _SessionInterface->DestroySession(NAME_GameSession))
	{
		_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(_DestroySessionCompleteHandle);
		_DestroySessionCompleteHandle.Reset();

		const EFPSSessionDestroyIntent FailedIntent = _DestroyIntent;
		_DestroyIntent = EFPSSessionDestroyIntent::None;

		SetOperationState(EFPSOnlineOperationState::Idle);

		const FString Error = TEXT("DestroySession request could not be started.");

		BroadcastDestroyFailure(FailedIntent, Error);
		return false;
	}

	return true;
}

void UFPSOnlineSessionSubsystem::BroadcastDestroyFailure(EFPSSessionDestroyIntent Intent, const FString& Error)
{
	switch (Intent)
	{
		case EFPSSessionDestroyIntent::Recreate:
		{
			ResetPendingCreate();
			_OnCreateSessionCompleted.Broadcast(false, Error);
			break;
		}
		case EFPSSessionDestroyIntent::JoinRollback:
		{
			SetConnectionState(EFPSOnlineConnectionState::Joined);

			FString JoinError;

			if (true == _PendingJoinError.IsEmpty())
			{
				JoinError = FString::Printf(TEXT("Joined session cleanup failed: %s"), *Error);
			}
			else
			{
				JoinError = FString::Printf(TEXT("%s Cleanup also failed: %s"), *_PendingJoinError, *Error);
			}

			_PendingJoinError.Reset();
			_OnJoinSessionCompleted.Broadcast(false, JoinError);
			break;
		}
		case EFPSSessionDestroyIntent::Leave:
		{
			_OnLeaveSessionCompleted.Broadcast(false, Error);
			break;
		}
		case EFPSSessionDestroyIntent::Destroy:
		{
			_OnDestroySessionCompleted.Broadcast(false, Error);
			break;
		}
		default:
		{
			UE_LOG(LogFPSOnlineSession, Warning, TEXT("Destroy failed without a valid intent: %s"), *Error);
			break;
		}
	}
}

EFPSOnlineOperationState UFPSOnlineSessionSubsystem::GetOperationState() const
{
	return _OperationState;
}

EFPSOnlineConnectionState UFPSOnlineSessionSubsystem::GetConnectionState() const
{
	return _ConnectionState;
}

bool UFPSOnlineSessionSubsystem::IsBusy() const 
{
	return EFPSOnlineOperationState::Idle != _OperationState;
}

bool UFPSOnlineSessionSubsystem::RequireIdle(const TCHAR* Operation, FString& OutError) const
{
	if (EFPSOnlineOperationState::Idle == _OperationState)
	{
		return true;
	}

	OutError = FString::Printf(TEXT("%s rejected: another operation is in progress."), Operation);
	return false;
}

void UFPSOnlineSessionSubsystem::SetOperationState(EFPSOnlineOperationState NewState)
{
	if (_OperationState == NewState)
	{
		return;
	}

	_OperationState = NewState;
	_OnOperationStateChanged.Broadcast(NewState);
}

void UFPSOnlineSessionSubsystem::SetConnectionState(EFPSOnlineConnectionState NewState)
{
	if (_ConnectionState == NewState)
	{
		return;
	}

	_ConnectionState = NewState;
	_OnConnectionStateChanged.Broadcast(NewState);
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
}




