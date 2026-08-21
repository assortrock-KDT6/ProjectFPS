// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Common/FPSOnlineSessionKeys.h"

DEFINE_LOG_CATEGORY(LogFPSOnlineSession);

 /** 
 * 언리얼은 Unity 빌드를 실행해서 namespace로 묶어두지 않으면
 * 전체 클래스에서 동일한 함수명을 사용하는 함수가 발생시
 * 오류가 난다.
 */
namespace OnlineSessionSubsystemUtils
{
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
		return Subsystem && Subsystem->GetSubsystemName() == NULL_SUBSYSTEM;
	}

	bool IsFatalHostNetworkFailure(ENetworkFailure::Type FailureType)
	{
		switch (FailureType)
		{
			case ENetworkFailure::NetDriverAlreadyExists:
			case ENetworkFailure::NetDriverCreateFailure:
			case ENetworkFailure::NetDriverListenFailure:
			{
				// Listen Server 자체가 접속을 받을 수 없는 경우에만 해당한다.
				return true;
			}
			default:
			{
				// 개별 Client 연결 실패로 Host 세션을 내리지 않는다.
				return false;
			}
		}
	}
}

void UFPSOnlineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	_OperationState  = EFPSOnlineOperationState::Idle;
	_ConnectionState = EFPSOnlineConnectionState::None;
	_TravelState	 = EFPSOnlineTravelState::None;
	_TravelIntent	 = EFPSSessionTravelIntent::None;

	_PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMapWithWorld);

	if (true == IsValid(GEngine))
	{
		_TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
		_NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
	}

#if !UE_BUILD_SHIPPING
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		const FString SubsystemName = Subsystem->GetSubsystemName().ToString();
		UE_LOG(LogFPSOnlineSession, Log, TEXT("Initialize: OSS = %s"), *SubsystemName);
	}
#endif // !UE_BUILD_SHIPPING
}

bool UFPSOnlineSessionSubsystem::RefreshSessionInterface()
{
	_SessionInterface.Reset();

	UWorld* World = GetWorld();
	if (false == IsValid(World))
	{
		return false;
	}

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
	if (true == _PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapHandle);
		_PostLoadMapHandle.Reset();
	}
	if (true == IsValid(GEngine))
	{
		if (true == _TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(_TravelFailureHandle);
			_TravelFailureHandle.Reset();
		}
		if (true == _NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(_NetworkFailureHandle);
			_NetworkFailureHandle.Reset();
		}
	}
	if (nullptr != GetGameInstance())
	{
		FTimerManager& TimerManager = GetGameInstance()->GetTimerManager();

		TimerManager.ClearTimer(_TravelWatchdogHandle);
		TimerManager.ClearTimer(_OperationWatchdogHandle);
	}

	// 비동기 완료 Callback이 종료 중인 Subsystem으로 돌아오지않게 먼저 해제한다.
	ClearOnlineSessionDelegates();

	// 종료 중이므로 비동기 콜백을 기다리지 않는다.
	if (true == _SessionInterface.IsValid() && nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		_SessionInterface->DestroySession(NAME_GameSession);
	}

	_SessionSearch.Reset();
	_SessionInterface.Reset();
	_PendingMatchMapPath.Reset();
	_RecoveryPending = false;

	ResetPendingCreate();
	ResetAutoMatch();
	_PendingJoinError.Reset();

	_DestroyIntent		= EFPSSessionDestroyIntent::None;
	_OperationState		= EFPSOnlineOperationState::Idle;
	_ConnectionState	= EFPSOnlineConnectionState::None;
	_TravelState		= EFPSOnlineTravelState::None;
	_TravelIntent		= EFPSSessionTravelIntent::None;

	Super::Deinitialize();
}

bool UFPSOnlineSessionSubsystem::CreateSession(const FFPSSessionCreateOptions& Options)
{
	FString ErrorMessage;
	if (false == RequireIdle(TEXT("CreateSession"), ErrorMessage))
	{
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::Joined == _ConnectionState ||
		EFPSOnlineConnectionState::CleanupFailed == _ConnectionState)
	{
		ErrorMessage = TEXT("Clean up the current local session before creating a session.");
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}

	if (Options._MaxPlayers <= 0)
	{
		ErrorMessage = TEXT("PublicConnections must be greater than zero.");
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}


	FFPSSessionCreateOptions ValidatedOptions = Options;
	ValidatedOptions._MapId.TrimStartAndEndInline();
	ValidatedOptions._DisplayName.TrimStartAndEndInline();
	ValidatedOptions._GameModeId.TrimStartAndEndInline();

	if (false == CanServerTravel(ValidatedOptions._MapId, ErrorMessage))
	{
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}

	_PendingCreateOptions = MoveTemp(ValidatedOptions);
	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface unavailable.");
		ResetPendingCreate();
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}

	if (nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		return StartDestroySession(EFPSSessionDestroyIntent::Recreate);
	}
	return StartCreateSession();
}

bool UFPSOnlineSessionSubsystem::StartMatch(const FString& MapPath)
{
	FString ErrorMessage;
	if (false == RequireIdle(TEXT("StartMatch"), ErrorMessage))
	{
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::Hosting != _ConnectionState)
	{
		ErrorMessage = TEXT("Only the host can start the match.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface is unavailable.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return false;
	}

	FString PendingMapPath = MapPath;
	PendingMapPath.TrimStartAndEndInline();

	if (false == CanServerTravel(PendingMapPath, ErrorMessage))
	{
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return false;
	}

	const FNamedOnlineSession* NamedSession = _SessionInterface->GetNamedSession(NAME_GameSession);

	if (nullptr == NamedSession)
	{
		ErrorMessage = TEXT("The hosted session no longer exists.");
		SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EOnlineSessionState::InProgress == NamedSession->SessionState)
	{
		const bool Started = TravelHostToGame(PendingMapPath, EFPSSessionTravelIntent::Host, ErrorMessage);
		if (false == Started)
		{
			_OnTravelFailed.Broadcast(ErrorMessage);
			_OnMatchStarted.Broadcast(false, ErrorMessage);
		}
		return Started;
	}

	if (EOnlineSessionState::Pending != NamedSession->SessionState && EOnlineSessionState::Ended != NamedSession->SessionState)
	{
		ErrorMessage = FString::Printf(TEXT("Session cannot start from state %s."), EOnlineSessionState::ToString(NamedSession->SessionState));
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (true == _StartSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(_StartSessionCompleteHandle);
		_StartSessionCompleteHandle.Reset();
	}

	/**
	* NULL OSS는 완료 Delegate를 StartSession() 내부에서 동기 발화한다.
	* 따라서 Pending 데이터와 State를 요청 전에 모두 확정해두어야한다.
	*/

	_PendingMatchMapPath = MoveTemp(PendingMapPath);
	_StartSessionCompleteHandle = _SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleStartSessionComplete));
	SetOperationState(EFPSOnlineOperationState::Starting);

	const bool RequestStarted = _SessionInterface->StartSession(NAME_GameSession);
	if (false == RequestStarted && true == _StartSessionCompleteHandle.IsValid())
	{
		/**
		* 일부 OSS(Online Subsystem)는 false를 반환하면서 동기 Callback을 이미 방송했다.
		* Handle이 아직 유효할 때만 *Callback이 오지 않은 시작실패*로 보고 직접 정리한다.
		*/

		_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(_StartSessionCompleteHandle);
		_StartSessionCompleteHandle.Reset();
		_PendingMatchMapPath.Reset();
		SetOperationState(EFPSOnlineOperationState::Idle);

		ErrorMessage = TEXT("StartSession request could not be started.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
	}

	return RequestStarted;
}

bool UFPSOnlineSessionSubsystem::StartCreateSession()
{
	if (false == _SessionInterface.IsValid())
	{
		const FString ErrorMessage = TEXT("Session interface before CreateSession.");
		ResetPendingCreate();
		SetOperationState(EFPSOnlineOperationState::Idle);
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = _PendingCreateOptions._MaxPlayers;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = _PendingCreateOptions._AllowJoinProgress;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bIsLANMatch = OnlineSessionSubsystemUtils::IsNullSubsystem(GetWorld());
	Settings.Set(SETTING_MAPNAME, _PendingCreateOptions._MapId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SETTING_FPS_DISPLAYNAME, _PendingCreateOptions._DisplayName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SETTING_GAMEMODE, _PendingCreateOptions._GameModeId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	

	if (_CreateSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
	}

	/* 
	 * 비동기 요청을 보내기 전에 Callback을 먼저 등록한다.
	 * 요청 이후 Delegate를 등록하면 구현에 따라 완료 이벤트를 놓칠 가능성이 있으므로
	 * Delegate 등록 -> State 변경 -> Request 순서를 유지한다.
	*/
	_CreateSessionCompleteHandle = _SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	SetOperationState(EFPSOnlineOperationState::Creating);

	if (false == _SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
		_CreateSessionCompleteHandle.Reset();
		ResetPendingCreate();
		SetOperationState(EFPSOnlineOperationState::Idle);

		const FString ErrorMessage = TEXT("CreateSession request could not be started.");
		BroadcastCreateCompleted(false, ErrorMessage);
		return false;
	}
	return true;
}

void UFPSOnlineSessionSubsystem::HandleStartSessionComplete(FName SessionName, bool WasSuccessful)
{
	if (NAME_GameSession != SessionName)
	{
		return;
	}

	if (true == _SessionInterface.IsValid() && true == _StartSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(_StartSessionCompleteHandle);
	}
	_StartSessionCompleteHandle.Reset();

	FString MapPath = MoveTemp(_PendingMatchMapPath);
	_PendingMatchMapPath.Reset();

	if (false == WasSuccessful)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);

		const FString ErrorMessage = TEXT("Online subsystem failed to start the session.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
		return;
	}

	UpdateAdvertisedMap(MapPath);

	FString ErrorMessage;
	const bool Started = TravelHostToGame(MapPath, EFPSSessionTravelIntent::Host, ErrorMessage);
	SetOperationState(EFPSOnlineOperationState::Idle);

	if (false == Started)
	{
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnMatchStarted.Broadcast(false, ErrorMessage);
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogFPSOnlineSession, Log, TEXT("StartSession completed. Match is now in progress"));
#endif
}

void UFPSOnlineSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool WasSuccessful)
{
	if (NAME_GameSession != SessionName)
	{
		return;
	}

	/* 
	 * 요청 하나당 Callback도 한 번만 처리한다.
	 * 완료된 Delegate를 즉시 해제하여 다음 요청에서 Callback이 중복 호출되는 것을 막는다.
	 */
	if (true == _SessionInterface.IsValid() && true == _CreateSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
	}
	_CreateSessionCompleteHandle.Reset();

	SetOperationState(EFPSOnlineOperationState::Idle);

	if (false == WasSuccessful)
	{
		ResetPendingCreate();
		BroadcastCreateCompleted(false, TEXT("Online subsystem failed to create the session."));
		return;
	}
		
	SetConnectionState(EFPSOnlineConnectionState::Hosting);

	/**
	* 세션이 광고되는 동안 접속을 받을 서버가 없으면
	* 그 사이 참가하는 Client가 모두 접속에 실패한다.
	* 따라서 생성 직후 곧바로 대기방 맵을 Listen Server로 연다.
	*/

	FString TravelError;
	const bool Started = TravelHostToGame(_PendingCreateOptions._MapId, EFPSSessionTravelIntent::HostLobby, TravelError);

	ResetPendingCreate();

	if (false == Started)
	{
		/**
		* 광고만 되고 접속을 받을 서버가 없는 상태이므로 세션을 즉시 회수한다.
		* 실패 알림보다 정리를 먼저 시작해 Destroying 동안 리스너의 새 요청을 막는다.
		* 단, 동기 완료하는 OSS(Null)에서는 이 시점에 이미 Idle이므로 차단이 성립되지 않는다. 
		* UI는 이벤트만 믿지 말고 OperationState도 확인해야한다.
		*/
		StartDestroySession(EFPSSessionDestroyIntent::HostLobbyRollback);
		BroadcastCreateCompleted(false, TravelError);
		_OnTravelFailed.Broadcast(TravelError);
		_OnLobbyReady.Broadcast(false, TravelError);
		return;
	}

	BroadcastCreateCompleted(true, FString());
}

bool UFPSOnlineSessionSubsystem::CanServerTravel(const FString& MapPath, FString& OutErrorMessage) const
{
	UWorld* World = GetWorld();

	if (false == IsValid(World))
	{
		OutErrorMessage = TEXT("World is unavailable.");
		return false;
	}

	if (NM_Client == World->GetNetMode())
	{
		OutErrorMessage = TEXT("Client cannot call ServerTravel.");
		return false;
	}

	if (false == World->NextURL.IsEmpty() || true == World->IsInSeamlessTravel())
	{
		OutErrorMessage = TEXT("Another world travel is already in progress.");
		return false;
	}

	if (true == MapPath.IsEmpty())
	{
		OutErrorMessage = TEXT("MapPath is Empty.");
		return false;
	}

	if (false == FPackageName::IsValidLongPackageName(MapPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Invalid Map Package path: %s"), *MapPath);
		return false;
	}

	if (false == FPackageName::DoesPackageExist(MapPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Map Package does not exist: %s"), *MapPath);
		return false;
	}

	return true;
}

bool UFPSOnlineSessionSubsystem::TravelHostToGame(const FString& MapPath, EFPSSessionTravelIntent Intent, FString& OutErrorMessage)
{
	UWorld* World = GetWorld();
	if (false == IsValid(World))
	{
		OutErrorMessage = TEXT("World is unavailable.");
		return false;
	}

	FString TravelURL = MapPath;
	/**
	 * Host가 이동한 Game World가 다른 Client의 접속을 받을 수 있도록
	 * Listen Server로 열기 위해 ?listen 옵션을 추가한다.
	 */
	if (false == TravelURL.Contains(TEXT("?listen"), ESearchCase::IgnoreCase))
	{
		TravelURL += TEXT("?listen");
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogFPSOnlineSession, Log, TEXT("ServerTravel: %s"), *TravelURL);
#endif // !UE_BUILD_SHIPPING

	if (false == CanServerTravel(MapPath, OutErrorMessage))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogFPSOnlineSession, Error, TEXT("ServerTravel was rejected: %s | %s"), *TravelURL, *OutErrorMessage);
#endif // !UE_BUILD_SHIPPING
		return false;
	}

	
	// State를 먼저 세팅한다.
	_TravelIntent = Intent;
	SetTravelState(EFPSOnlineTravelState::Traveling);

	const bool Returned = World->ServerTravel(TravelURL);
	const bool Queued = (false == World->NextURL.IsEmpty()) || World->IsInSeamlessTravel();
	if (false == Returned || false == Queued)
	{
		_TravelIntent = EFPSSessionTravelIntent::None;
		SetTravelState(EFPSOnlineTravelState::None);

		OutErrorMessage = FString::Printf(TEXT("ServerTravel rejected URL: %s"), *TravelURL);
		return false;
	}
	return true;

}

bool UFPSOnlineSessionSubsystem::FindSessions(int32 MaxResults)
{
	FString ErrorMessage;
	TArray<FFPSOnlineSessionInfo> EmptySessions;

	if (false == RequireIdle(TEXT("FindSessions"), ErrorMessage))
	{
		BroadcastFindCompleted(false, EmptySessions, ErrorMessage);
		return false;
	}

	if (MaxResults <= 0 || !RefreshSessionInterface())
	{
		if (MaxResults <= 0)
		{
			ErrorMessage = TEXT("MaxResults must be greater than zero.");
		}
		else
		{
			ErrorMessage = TEXT("Online Session interface is unavailable.");
		}
		BroadcastFindCompleted(false, EmptySessions, ErrorMessage);
		return false;
	}

	_SessionSearch = MakeShared<FOnlineSessionSearch>();
	_SessionSearch->MaxSearchResults = MaxResults;
	_SessionSearch->bIsLanQuery = OnlineSessionSubsystemUtils::IsNullSubsystem(GetWorld());
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
		ErrorMessage = TEXT("FindSessions request could not be started.");
		BroadcastFindCompleted(false, EmptySessions, ErrorMessage);
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
		BroadcastFindCompleted(false, Sessions, TEXT("Online subsystem failed to find sessions."));
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

		Result.Session.SessionSettings.Get(SETTING_FPS_DISPLAYNAME, Information._DisplayName);
		Result.Session.SessionSettings.Get(SETTING_GAMEMODE, Information._GameModeId);

		// 표시 이름이 비어 있으면 소유자 이름으로 대체한다.
		if (true == Information._DisplayName.IsEmpty())
		{
			Information._DisplayName = Information._SessionOwnerName;
		}
	}

	BroadcastFindCompleted(true, Sessions, FString());
}


bool UFPSOnlineSessionSubsystem::JoinSessionByIndex(int32 ResultIndex)
{
	FString ErrorMessage;

	if (false == RequireIdle(TEXT("JoinSession"), ErrorMessage))
	{
		BroadcastJoinCompleted(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::None != _ConnectionState)
	{
		ErrorMessage = TEXT("JoinSession requires a disconnected state.");
		BroadcastJoinCompleted(false, ErrorMessage);
		return false;
	}

	if (false == _SessionSearch.IsValid() ||
		false == _SessionSearch->SearchResults.IsValidIndex(ResultIndex) ||
		false == _SessionSearch->SearchResults[ResultIndex].IsValid())
	{
		ErrorMessage = TEXT("The selected session result is invalid");
		BroadcastJoinCompleted(false, ErrorMessage);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface is unavailable.");
		BroadcastJoinCompleted(false, ErrorMessage);
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

		ErrorMessage = TEXT("JoinSession request could not be started.");
		BroadcastJoinCompleted(false, ErrorMessage);
		return false;
	}

	return true;
}

bool UFPSOnlineSessionSubsystem::EndMatch(const FString& LobbyMapPath)
{
	FString ErrorMessage;
	
	if (false == RequireIdle(TEXT("EndMatch"), ErrorMessage))
	{
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::Hosting != _ConnectionState)
	{
		ErrorMessage = TEXT("Only the host can end the match.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface is unavailable.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return false;
	}

	FString PendingMapPath = LobbyMapPath;
	PendingMapPath.TrimStartAndEndInline();

	if (false == CanServerTravel(PendingMapPath, ErrorMessage))
	{
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return false;
	}

	const FNamedOnlineSession* NamedSession = _SessionInterface->GetNamedSession(NAME_GameSession);
	if (nullptr == NamedSession)
	{
		ErrorMessage = TEXT("The hosted session no longer exists.");
		SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return false;
	}

	// 이미 매치가 끝난 상태면 로비 복귀 Travel만 수행한다.
	if (EOnlineSessionState::InProgress != NamedSession->SessionState)
	{
		const bool Started = TravelHostToGame(PendingMapPath, EFPSSessionTravelIntent::HostLobby, ErrorMessage);
		if (false == Started)
		{
			_OnTravelFailed.Broadcast(ErrorMessage);
			_OnLobbyReady.Broadcast(false, ErrorMessage);
		}
		return Started;
	}

	if (true == _EndSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnEndSessionCompleteDelegate_Handle(_EndSessionCompleteHandle);
		_EndSessionCompleteHandle.Reset();
	}

	_PendingMatchMapPath = MoveTemp(PendingMapPath);
	_EndSessionCompleteHandle = _SessionInterface->AddOnEndSessionCompleteDelegate_Handle(
		FOnEndSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleEndSessionComplete));
	SetOperationState(EFPSOnlineOperationState::Ending);

	const bool RequestStarted = _SessionInterface->EndSession(NAME_GameSession);
	if (false == RequestStarted && true == _EndSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnEndSessionCompleteDelegate_Handle(_EndSessionCompleteHandle);
		_EndSessionCompleteHandle.Reset();
		_PendingMatchMapPath.Reset();
		SetOperationState(EFPSOnlineOperationState::Idle);

		ErrorMessage = TEXT("EndSession request could not be started.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
	}
	
	return RequestStarted;
}

void UFPSOnlineSessionSubsystem::HandleEndSessionComplete(FName SessionName, bool WasSuccessful)
{
	if (NAME_GameSession != SessionName)
	{
		return;
	}

	if (true == _SessionInterface.IsValid() && true == _EndSessionCompleteHandle.IsValid())
	{
		_SessionInterface->ClearOnEndSessionCompleteDelegate_Handle(_EndSessionCompleteHandle);
	}
	_EndSessionCompleteHandle.Reset();

	FString MapPath = MoveTemp(_PendingMatchMapPath);
	_PendingMatchMapPath.Reset();

	if (false == WasSuccessful)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);

		/**
		* 세션이 InProgress로 남아 있으므로 로비로 돌아가도록 난입 잠금이 풀리지 않는다.
		* 이동하지 않고 실패를 알려 EndMatch 재시도로 이어지게 한다.
		*/
		const FString ErrorMessage = TEXT("Online subsystem failed to end the session.");
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return;
	}

	UpdateAdvertisedMap(MapPath);

	FString ErrorMessage;
	const bool Started = TravelHostToGame(MapPath, EFPSSessionTravelIntent::HostLobby, ErrorMessage);
	
	SetOperationState(EFPSOnlineOperationState::Idle);

	if (false == Started)
	{
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
	}
}

/**
* JoinSession 성공은 OnlineSubsystem상의 Session 참가가 완료되었다는 뜻이다.
* 실제 게임 서버로 이동한 것은 아니다.
* 
* Session의 접속 주소를 얻은 뒤 ClientTravel을 수행해야 실제 Host의 World로 이동하게 된다.
*/
void UFPSOnlineSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (NAME_GameSession != SessionName)
	{
		return;
	}

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
		BroadcastJoinCompleted(false, OnlineSessionSubsystemUtils::JoinResultToError(Result));
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
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (false == IsValid(PlayerController) || false == PlayerController->IsLocalController())
		{
			RollbackJoinedSession(TEXT("Local PlayerController is unavailable."));
			return;
		}

		SetOperationState(EFPSOnlineOperationState::Idle);
		_TravelIntent = EFPSSessionTravelIntent::Join;
		SetTravelState(EFPSOnlineTravelState::Traveling);
		PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	}
	else
	{
		RollbackJoinedSession(TEXT("World is unavailable."));
	}
}

void UFPSOnlineSessionSubsystem::RollbackJoinedSession(const FString& ErrorMessage)
{
	_PendingJoinError = ErrorMessage;
	_TravelIntent = EFPSSessionTravelIntent::None;

	if (false == _SessionInterface.IsValid() && false == RefreshSessionInterface())
	{
		/**
		* 로컬 세션이 남아 있는지 확인할 수 없다.
		* None으로 확정하면 남은 세션을 영원히 정리하지 못하므로 CleanupFailed로 표시한다.
		*/

		SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
		SetOperationState(EFPSOnlineOperationState::Idle);
		SetTravelState(EFPSOnlineTravelState::None);

		const FString JoinError = FString::Printf(TEXT("%s Local joined-session cleanup could not be verified: "
			"the session interface is unavailable."), *_PendingJoinError);
		_PendingJoinError.Reset();
		BroadcastJoinCompleted(false, JoinError);
		return;
	}

	if (nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		if (true == StartDestroySession(EFPSSessionDestroyIntent::JoinRollback))
		{
			SetTravelState(EFPSOnlineTravelState::None);
		}
		return;
	}

	SetConnectionState(EFPSOnlineConnectionState::None);
	SetOperationState(EFPSOnlineOperationState::Idle);
	SetTravelState(EFPSOnlineTravelState::None);

	FString JoinError = MoveTemp(_PendingJoinError);
	_PendingJoinError.Reset();
	BroadcastJoinCompleted(false, JoinError);
}

bool UFPSOnlineSessionSubsystem::LeaveSession()
{
	FString ErrorMessage;
	if (false == RequireIdle(TEXT("LeaveSession"), ErrorMessage))
	{
		_OnLeaveSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::Hosting == _ConnectionState)
	{
		ErrorMessage = TEXT("A host must use DestroySession instead of LeaveSession.");
		_OnLeaveSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface is unavailable.");
		_OnLeaveSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (nullptr == _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SetConnectionState(EFPSOnlineConnectionState::None);
		_OnLeaveSessionCompleted.Broadcast(true, FString());
		return true;
	}

	/*
	* OnlineSubsystem에서는 Client의 Local NamedSession 정리도 DestroySession API를 통해 수행한다.
	* 여기서 Destroy는 Host의 방 자체를 없앤다는 의미가 아니라, 현재 Local User가 가지고 있는 Session 상태를 정리하는 의미로 사용한다.
	* 실제 목적은 DestroyIntent::Leave로 구분한다.
	*/
	return StartDestroySession(EFPSSessionDestroyIntent::Leave);
}

bool UFPSOnlineSessionSubsystem::DestroySession()
{
	FString ErrorMessage;
	if (false == RequireIdle(TEXT("DestroySession"), ErrorMessage))
	{
		_OnDestroySessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::Joined == _ConnectionState)
	{
		ErrorMessage = TEXT("A joined client must use LeaveSession.");
		_OnDestroySessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface is unavailable.");
		_OnDestroySessionCompleted.Broadcast(false, ErrorMessage);
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
	if (NAME_GameSession != SessionName)
	{
		return;
	}

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
		const FString ErrorMessage = TEXT("Online subsystem failed to destroy the session.");

		BroadcastDestroyFailure(CompletedIntent, ErrorMessage);
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

		BroadcastJoinCompleted(false, JoinError);
	}
	else if (EFPSSessionDestroyIntent::HostLobbyRollback == CompletedIntent)
	{
		// 실패는 이미 _OnCreateSessionCompleted / _OnLobbyReady로 알렸다.
		// 정리 완료는 ConnectionState(None)과 OperationState(Idle)로 관찰한다.
		SetOperationState(EFPSOnlineOperationState::Idle);
	}
	else if (EFPSSessionDestroyIntent::DisconnectRecovery == CompletedIntent)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
		// _OnConnectionLost와 ConnectionState(None)가 이미 결과를 전달한다.
		// 명시적인 LeaveSession 요청이 아니므로 _OnLeaveSessionCompleted는 방송하지 않는다.
	}
	else
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
	}
}

void UFPSOnlineSessionSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (EFPSOnlineTravelState::Traveling != _TravelState ||
		false == IsValid(LoadedWorld) ||
		LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const EFPSSessionTravelIntent CompletedIntent = _TravelIntent;
	const ENetMode LoadedNetMode = LoadedWorld->GetNetMode();

	if (EFPSSessionTravelIntent::Join == CompletedIntent &&
		NM_Client != LoadedNetMode)
	{
		return;
	}

	/**
	* Host Travel의 목적은 ?listen 옵션으로 Listen Server를 여는 것이다.
	* NetDriver 생성이나 Port Binding에 실패하면 World가 Standalone으로 열리는데,
	* 이때 Session은 광고되지만 아무도 접속할 수 없는 상태가 된다.
	* NetMode를 확인하지 않고 성공으로 통지하면 Host는 정상으로 보이고 참가자만 전부 실패한다.
	*/
	if ((EFPSSessionTravelIntent::HostLobby == CompletedIntent || EFPSSessionTravelIntent::Host == CompletedIntent) &&
		NM_ListenServer != LoadedNetMode && NM_DedicatedServer != LoadedNetMode)
	{
		const FString ErrorMessage = FString::Printf(
			TEXT("Host travel finished without a listen server (NetMode = %d)."), static_cast<int32>(LoadedNetMode));

		UE_LOG(LogFPSOnlineSession, Error, TEXT("%s"), *ErrorMessage);

		// HostLobby면 광고만 되는 Session을 회수하고, 두 경우 모두 실패를 통지한다.
		HandleHostTravelFailure(CompletedIntent, ErrorMessage);
		return;
	}

	_TravelIntent = EFPSSessionTravelIntent::None;

	if (EFPSSessionTravelIntent::Join == CompletedIntent)
	{
		SetConnectionState(EFPSOnlineConnectionState::Joined);
	}

	SetTravelState(EFPSOnlineTravelState::None);

	if (EFPSSessionTravelIntent::Join == CompletedIntent)
	{
		// 참가에 사용한 검색 결과는 더 이상 유효하지 않다. 재입장 시 반드시 다시 검색하게 한다.
		_SessionSearch.Reset();
		BroadcastJoinCompleted(true, FString());
	}
	else if (EFPSSessionTravelIntent::HostLobby == CompletedIntent)
	{
		_OnLobbyReady.Broadcast(true, FString());
	}
	else if (EFPSSessionTravelIntent::Host == CompletedIntent)
	{
		_OnMatchStarted.Broadcast(true, FString());
	}
}

void UFPSOnlineSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorMessage)
{
	if (true == IsValid(World) && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const FString TravelError = FString::Printf(TEXT("Travel failure [%d]: %s"), static_cast<int32>(FailureType), *ErrorMessage);

	if (EFPSOnlineTravelState::Traveling != _TravelState)
	{
		/**
		* Host가 StartMatch / EndMatch로 ServerTravel하면 Client도 따라 이동하지만,
		* Client는 자신이 Travel을 시작한 것이 아니므로 _TravelState가 Traveling이 아니다.
		* 이 경우를 그냥 무시하면 이동에 실패해도 ConnectionState가 Joined로 남아
		* 실제로는 Host와 끊긴 Player가 다시 Session에 참가할 수 없게 된다.
		*/
		if (EFPSOnlineConnectionState::Joined == _ConnectionState)
		{
			UE_LOG(LogFPSOnlineSession, Error, TEXT("Travel failed while following the host: %s"), *TravelError);
			BeginDisconnectRecovery(TravelError);
		}
		return;
	}

	if (EFPSSessionTravelIntent::Join == _TravelIntent)
	{
		HandleJoinTravelFailure(TravelError);
		return;
	}

	HandleHostTravelFailure(_TravelIntent, TravelError);
}

void UFPSOnlineSessionSubsystem::HandleJoinTravelFailure(const FString& ErrorMessage)
{
	if (EFPSOnlineTravelState::Traveling != _TravelState)
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogFPSOnlineSession, Error, TEXT("Join travel failed: %s"), *ErrorMessage);
#endif

	_OnTravelFailed.Broadcast(ErrorMessage);
	RollbackJoinedSession(ErrorMessage);
}

void UFPSOnlineSessionSubsystem::HandleHostTravelFailure(EFPSSessionTravelIntent FailedIntent, const FString& ErrorMessage)
{
	_TravelIntent = EFPSSessionTravelIntent::None;
	SetTravelState(EFPSOnlineTravelState::None);

	if (EFPSSessionTravelIntent::HostLobby == FailedIntent)
	{
		/**
		* Listen Server를 열지 못했으면 세션만 광고되는 위험 상태이므로 정리한다.
		* 정리를 먼저 시작해야 OperationState가 Destroying이 되어
		* 아래 Broadcast를 받은 리스너가 새 작업을 시작하지 못한다.
		*/

		StartDestroySession(EFPSSessionDestroyIntent::HostLobbyRollback);
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return;
	}

	_OnTravelFailed.Broadcast(ErrorMessage);
	_OnMatchStarted.Broadcast(false, ErrorMessage);

}

bool UFPSOnlineSessionSubsystem::IsTravelStillInProgress() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (nullptr != GameInstance)
	{
		const FWorldContext* WorldContext = GameInstance->GetWorldContext();
		if (nullptr != WorldContext && nullptr != WorldContext->PendingNetGame)
		{
			// Client가 아직 Host에 접속 중이거나 Map을 내려받는 중이다.
			return true;
		}
	}

	const UWorld* World = GetWorld();
	if (true == IsValid(World))
	{
		if (false == World->NextURL.IsEmpty() || true == World->IsInSeamlessTravel())
		{
			// ServerTravel이 예약되었으나 아직 수행되지 않았다.
			return true;
		}
	}

	return false;
}

void UFPSOnlineSessionSubsystem::HandleTravelTimeout()
{
	if (EFPSOnlineTravelState::Traveling != _TravelState)
	{
		return;
	}

	/**
	* 대형 Map 로딩이나 접속 협상은 정상적인 경우에도 Watchdog 시간을 넘길 수 있다.
	* 아직 진행 중이라는 근거가 있으면 실패로 확정하지 않고 감시 시간을 연장한다.
	* 무한 대기를 막기 위해 연장 횟수는 _MaxTravelWatchdogExtensions로 제한한다.
	*/
	if (true == IsTravelStillInProgress() && _TravelWatchdogExtensions < _MaxTravelWatchdogExtensions)
	{
		UGameInstance* GameInstance = GetGameInstance();
		if (true == IsValid(GameInstance))
		{
			++_TravelWatchdogExtensions;

			UE_LOG(LogFPSOnlineSession, Warning,
				TEXT("Travel watchdog extended (%d/%d): travel is still in progress."),
				_TravelWatchdogExtensions, _MaxTravelWatchdogExtensions);

			GameInstance->GetTimerManager().SetTimer(
				_TravelWatchdogHandle, this, &ThisClass::HandleTravelTimeout, _TravelTimeoutSeconds, false);
			return;
		}
	}

	const FString ErrorMessage = TEXT("Travel timed out without any completion or failure event.");

	const EFPSSessionTravelIntent TimedOutIntent = _TravelIntent;
	UE_LOG(LogFPSOnlineSession, Error, TEXT("%s (Intent=%d)"), *ErrorMessage, static_cast<int32>(TimedOutIntent));

	if (EFPSSessionTravelIntent::Join == TimedOutIntent)
	{
		HandleJoinTravelFailure(ErrorMessage);
	}
	else
	{
		HandleHostTravelFailure(TimedOutIntent, ErrorMessage);
	}
}

void UFPSOnlineSessionSubsystem::HandleOperationTimeout()
{
	const EFPSOnlineOperationState TimedOutState = _OperationState;
	if (EFPSOnlineOperationState::Idle == TimedOutState)
	{
		return;
	}

	UE_LOG(LogFPSOnlineSession, Error, TEXT("Online operation timed out (State = %d). Forcing Idle."), static_cast<int32>(TimedOutState));

	/**
	* 뒤늦게 도착하는 Callback이 이미 정리된 상태를 다시 건드리지 않도록 등록된 Session Delegate를 모두 해제한다.
	*/
	ClearOnlineSessionDelegates();

	const EFPSSessionDestroyIntent TimeOutIntent = _DestroyIntent;
	_DestroyIntent = EFPSSessionDestroyIntent::None;
	_PendingMatchMapPath.Reset();

	SetOperationState(EFPSOnlineOperationState::Idle);

	const FString ErrorMessage = TEXT("Online subsystem did not respond in time.");
	
	switch (TimedOutState)
	{
		case EFPSOnlineOperationState::Destroying:
		{
			BroadcastDestroyFailure(TimeOutIntent, ErrorMessage);
			break;
		}
		case EFPSOnlineOperationState::Creating:
		{
			ResetPendingCreate();
			BroadcastCreateCompleted(false, ErrorMessage);
			break;
		}
		case EFPSOnlineOperationState::Finding:
		{
			TArray<FFPSOnlineSessionInfo> EmptySessions;
			BroadcastFindCompleted(false, EmptySessions, ErrorMessage);
			break;
		}
		case EFPSOnlineOperationState::Joining:
		{
			SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
			_PendingJoinError.Reset();
			BroadcastJoinCompleted(false, ErrorMessage);
			break;
		}
		case EFPSOnlineOperationState::Starting:
		{
			_OnTravelFailed.Broadcast(ErrorMessage);
			_OnMatchStarted.Broadcast(false, ErrorMessage);
			break;
		}
		case EFPSOnlineOperationState::Ending:
		{
			_OnTravelFailed.Broadcast(ErrorMessage);
			_OnLobbyReady.Broadcast(false, ErrorMessage);
			break;
		}
	}
}

void UFPSOnlineSessionSubsystem::BeginDisconnectRecovery(const FString& ErrorMessage)
{
	if (EFPSOnlineOperationState::Idle != _OperationState)
	{
		UE_LOG(LogFPSOnlineSession, Warning, TEXT("Disconnect recovery deferred: operation in progress"));

		// Operation 이 Idle로 돌아오는 시점에 SetOperationState가 재시도를 예약한다.
		_RecoveryPending = true;
		SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
		_OnConnectionLost.Broadcast(ErrorMessage);
		return;
	}

	if (false == RefreshSessionInterface())
	{
		SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
		_OnConnectionLost.Broadcast(ErrorMessage);
		return;
	}

	if (nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		StartDestroySession(EFPSSessionDestroyIntent::DisconnectRecovery);
	}
	else
	{
		SetConnectionState(EFPSOnlineConnectionState::None);
	}

	_OnConnectionLost.Broadcast(ErrorMessage);
}

void UFPSOnlineSessionSubsystem::RetryDisconnectRecovery()
{
	/**
	* 이미 정리가 끝났거나(_RecoveryPending == false) 다른 Operation이 진행 중이면 아무것도 하지 않는다.
	* SetOperationState가 Idle 전환마다 예약하므로 이 함수는 중복 예약될 수 있다.
	* 여기서 상태를 건드리면 이미 None으로 정리된 ConnectionState를 되돌리게 된다.
	* Operation이 진행 중인 경우에는 _RecoveryPending을 유지해 다음 Idle 전환에서 다시 시도한다.
	*/
	if (false == _RecoveryPending || EFPSOnlineOperationState::Idle != _OperationState)
	{
		return;
	}

	_RecoveryPending = false;

	if (false == RefreshSessionInterface())
	{
		SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
		return;
	}

	if (nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		StartDestroySession(EFPSSessionDestroyIntent::DisconnectRecovery);
		return;
	}

	SetConnectionState(EFPSOnlineConnectionState::None);
}

void UFPSOnlineSessionSubsystem::UpdateAdvertisedMap(const FString& MapPath)
{
	if (false == _SessionInterface.IsValid())
	{
		return;
	}

	FOnlineSessionSettings* Settings = _SessionInterface->GetSessionSettings(NAME_GameSession);
	
	if (nullptr == Settings)
	{
		return;
	}

	Settings->Set(SETTING_MAPNAME, MapPath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	/**
	* 실패해도 게임 진행에는 영향이 없고 광고 정보만 낡은 채로 남는다.
	* 별도 완료 Callback을 처리하지 않고 결과만 기록한다.
	*/
	if (false == _SessionInterface->UpdateSession(NAME_GameSession, *Settings, true))
	{
		UE_LOG(LogFPSOnlineSession, Warning, TEXT("Failed to update the advertised map name: %s"), *MapPath);
	}
}

void UFPSOnlineSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorMessage)
{
	if (true == IsValid(World) && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	
	/**
	* Beacon 등 GameNetDriver가 아닌 실패는 Session 상태와 무관하다.
	*/
	if (nullptr != NetDriver && NAME_GameNetDriver != NetDriver->NetDriverName)
	{
		return;
	}

	const FString NetworkError = FString::Printf(TEXT("Network failure [%d]: %s"), static_cast<int32>(FailureType), *ErrorMessage);

	if (EFPSOnlineTravelState::Traveling == _TravelState)
	{
		if (EFPSSessionTravelIntent::Join == _TravelIntent)
		{
			HandleJoinTravelFailure(NetworkError);
		}
		else
		{
			HandleHostTravelFailure(_TravelIntent, NetworkError);
		}
		return;
	}

	/**
	* 이미 세션 정리가 진행 중이면 같은 실패를 중복 처리하지 않는다.
	*/

	if (EFPSOnlineOperationState::Destroying == _OperationState)
	{
		return;
	}

	if (EFPSOnlineConnectionState::Joined == _ConnectionState)
	{
		BeginDisconnectRecovery(NetworkError);
	}
	else if (EFPSOnlineConnectionState::Hosting == _ConnectionState && true == OnlineSessionSubsystemUtils::IsFatalHostNetworkFailure(FailureType))
	{
		/**
		* 접속 받을 수 없는 Listen Server가 세션만 광고하는 상태를 막는다.
		*/
		BeginDisconnectRecovery(NetworkError);
	}
}


bool UFPSOnlineSessionSubsystem::StartDestroySession(EFPSSessionDestroyIntent Intent)
{
	if (false == _SessionInterface.IsValid())
	{
		const FString ErrorMessage = TEXT("Online session interface is unavailable.");
		SetOperationState(EFPSOnlineOperationState::Idle);
		BroadcastDestroyFailure(Intent, ErrorMessage);
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

		const FString ErrorMessage = TEXT("DestroySession request could not be started.");

		BroadcastDestroyFailure(FailedIntent, ErrorMessage);
		return false;
	}

	return true;
}

void UFPSOnlineSessionSubsystem::BroadcastDestroyFailure(EFPSSessionDestroyIntent Intent, const FString& ErrorMessage)
{
	switch (Intent)
	{
		case EFPSSessionDestroyIntent::Recreate:
		{
			ResetPendingCreate();
			BroadcastCreateCompleted(false, ErrorMessage);
			break;
		}
		case EFPSSessionDestroyIntent::JoinRollback:
		{
			_TravelIntent = EFPSSessionTravelIntent::None;

			SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
			SetTravelState(EFPSOnlineTravelState::None);

			FString JoinError;
			if (true == _PendingJoinError.IsEmpty())
			{
				JoinError = FString::Printf(TEXT("Joined session cleanup failed: %s"), *ErrorMessage);
			}
			else
			{
				JoinError = FString::Printf(TEXT("%s Cleanup also failed: %s"), *_PendingJoinError, *ErrorMessage);
			}

			_PendingJoinError.Reset();
			BroadcastJoinCompleted(false, JoinError);
			break;
		}
		case EFPSSessionDestroyIntent::Leave:
		{
			_OnLeaveSessionCompleted.Broadcast(false, ErrorMessage);
			break;
		}
		case EFPSSessionDestroyIntent::HostLobbyRollback:
		{
			SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
			UE_LOG(LogFPSOnlineSession, Error, TEXT("Failed to clean up a session after lobby travel failure:%s"), *ErrorMessage);
			break;
		}
		case EFPSSessionDestroyIntent::Destroy:
		{
			_OnDestroySessionCompleted.Broadcast(false, ErrorMessage);
			break;
		}
		case EFPSSessionDestroyIntent::DisconnectRecovery:
		{
			SetConnectionState(EFPSOnlineConnectionState::CleanupFailed);
			UE_LOG(LogFPSOnlineSession, Error, TEXT("Disconnected session cleanup failed: %s"), *ErrorMessage);
			break;
		}
		default:
		{
			UE_LOG(LogFPSOnlineSession, Warning, TEXT("Destroy failed without a valid intent: %s"), *ErrorMessage);
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
	return  EFPSOnlineOperationState::Idle != _OperationState || 
			EFPSOnlineTravelState::Traveling == _TravelState;
}

bool UFPSOnlineSessionSubsystem::RequireIdle(const TCHAR* Operation, FString& OutErrorMessage) const
{
	if (false == IsBusy())
	{
		return true;
	}

	OutErrorMessage = FString::Printf(TEXT("%s rejected: another operation is in progress."), Operation);
	return false;
}

void UFPSOnlineSessionSubsystem::SetOperationState(EFPSOnlineOperationState NewState)
{
	if (_OperationState == NewState)
	{
		return;
	}

	_OperationState = NewState;

	// 레벨 전환을 넘어 유지되는 GameInstance의 TimerManager를 사용한다.
	UGameInstance* GameInstance = GetGameInstance();
	if (true == IsValid(GameInstance))
	{
		FTimerManager& TimerManager = GameInstance->GetTimerManager();
		TimerManager.ClearTimer(_OperationWatchdogHandle);

		if (EFPSOnlineOperationState::Idle != NewState)
		{
			TimerManager.SetTimer(_OperationWatchdogHandle, this, &ThisClass::HandleOperationTimeout, _OperationTimeoutSeconds, false);
		}
	}
	_OnOperationStateChanged.Broadcast(NewState);

	// 보류된 Disconnect 정리가 있으면 현재 Callback 스택을 벗어난 뒤 재시도한다.
	if (EFPSOnlineOperationState::Idle == NewState && true == _RecoveryPending && true == IsValid(GameInstance))
	{
		GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::RetryDisconnectRecovery);
	}
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

void UFPSOnlineSessionSubsystem::SetTravelState(EFPSOnlineTravelState NewState)
{
	if (_TravelState == NewState)
	{
		return;
	}
	
	_TravelState = NewState;

	// 새로운 Travel이 시작되거나 끝났으므로 Watchdog 연장 횟수를 초기화한다.
	_TravelWatchdogExtensions = 0;

	// 레벨 전환을 넘어 유지되는 GameInstance의 TimerManager를 사용한다.
	UGameInstance* GameInstance = GetGameInstance();
	if (true == IsValid(GameInstance))
	{
		FTimerManager& TimerManager = GameInstance->GetTimerManager();
		TimerManager.ClearTimer(_TravelWatchdogHandle);
		if (EFPSOnlineTravelState::Traveling == NewState)
		{
			TimerManager.SetTimer(_TravelWatchdogHandle, this, &ThisClass::HandleTravelTimeout, _TravelTimeoutSeconds, false);
		}
	}

	_OnTravelStateChanged.Broadcast(NewState);
}

EFPSOnlineTravelState UFPSOnlineSessionSubsystem::GetTravelState() const
{
	return _TravelState;
}

void UFPSOnlineSessionSubsystem::ClearOnlineSessionDelegates()
{
	if (true == _SessionInterface.IsValid())
	{
		if (true == _CreateSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(_CreateSessionCompleteHandle);
		}
		if (true == _StartSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(_StartSessionCompleteHandle);
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
		if (true == _EndSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnEndSessionCompleteDelegate_Handle(_EndSessionCompleteHandle);
		}
	}

	_CreateSessionCompleteHandle.Reset();
	_StartSessionCompleteHandle.Reset();
	_FindSessionCompleteHandle.Reset();
	_JoinSessionCompleteHandle.Reset();
	_DestroySessionCompleteHandle.Reset();
	_EndSessionCompleteHandle.Reset();
}

void UFPSOnlineSessionSubsystem::ResetPendingCreate()
{
	_PendingCreateOptions = FFPSSessionCreateOptions{};
}

#pragma region AutoMatch

void UFPSOnlineSessionSubsystem::BroadcastCreateCompleted(bool WasSuccessful, const FString& ErrorMessage)
{
	_OnCreateSessionCompleted.Broadcast(WasSuccessful, ErrorMessage);

	if (EFPSSessionAutoMatchStage::Hosting == _AutoMatchStage)
	{
		FinishAutoMatch(WasSuccessful, true, ErrorMessage);
	}
}

void UFPSOnlineSessionSubsystem::BroadcastFindCompleted(bool WasSuccessful, const TArray<FFPSOnlineSessionInfo>& Sessions, const FString& ErrorMessage)
{
	_OnFindSessionCompleted.Broadcast(WasSuccessful, Sessions, ErrorMessage);

	if (EFPSSessionAutoMatchStage::Finding == _AutoMatchStage)
	{
		ContinueAutoMatchAfterFind(WasSuccessful, Sessions);
	}
}

void UFPSOnlineSessionSubsystem::BroadcastJoinCompleted(bool WasSuccessful, const FString& ErrorMessage)
{
	if (EFPSSessionAutoMatchStage::Joining == _AutoMatchStage && false == WasSuccessful)
	{
		/**
		* 자동 매치 중의 개별 참가 실패는 다음 후보 또는 Host 전환으로 이어지는 중간 과정이다.
		* UI에 실패로 알리면 최종적으로 성공하는 흐름에서도 에러가 표시되므로 통지하지 않는다.
		*/
		AdvanceAutoMatchAfterJoinFailure(ErrorMessage);
		return;
	}

	_OnJoinSessionCompleted.Broadcast(WasSuccessful, ErrorMessage);

	if (EFPSSessionAutoMatchStage::Joining == _AutoMatchStage)
	{
		FinishAutoMatch(true, false, FString());
	}
}

bool UFPSOnlineSessionSubsystem::AutoJoinOrHost(const FFPSSessionCreateOptions& HostOptions, int32 MaxResults)
{
	FString ErrorMessage;

	if (false == RequireIdle(TEXT("AutoJoinOrHost"), ErrorMessage))
	{
		_OnAutoMatchCompleted.Broadcast(false, false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::None != _ConnectionState)
	{
		ErrorMessage = TEXT("AutoJoinOrHost requires a disconnected state.");
		_OnAutoMatchCompleted.Broadcast(false, false, ErrorMessage);
		return false;
	}

	if (HostOptions._MaxPlayers <= 0)
	{
		ErrorMessage = TEXT("PublicConnections must be greater than zero.");
		_OnAutoMatchCompleted.Broadcast(false, false, ErrorMessage);
		return false;
	}

	FFPSSessionCreateOptions ValidatedOptions = HostOptions;
	ValidatedOptions._MapId.TrimStartAndEndInline();
	ValidatedOptions._DisplayName.TrimStartAndEndInline();
	ValidatedOptions._GameModeId.TrimStartAndEndInline();

	/**
	* Host로 전환될 가능성이 있으므로 Map 유효성을 검색 전에 미리 확인한다.
	* 검색이 끝난 뒤에야 Map 문제로 실패하면 대기 시간만 낭비된다.
	*/
	if (false == CanServerTravel(ValidatedOptions._MapId, ErrorMessage))
	{
		_OnAutoMatchCompleted.Broadcast(false, false, ErrorMessage);
		return false;
	}

	ResetAutoMatch();
	_AutoMatchHostOptions = MoveTemp(ValidatedOptions);
	_AutoMatchStage = EFPSSessionAutoMatchStage::Finding;

	UE_LOG(LogFPSOnlineSession, Log, TEXT("AutoMatch: searching for a joinable session."));

	/**
	* FindSessions가 시작에 실패하더라도 완료 통지(BroadcastFindCompleted)가 동기적으로 호출되어
	* ContinueAutoMatchAfterFind가 Host 전환까지 처리한다. 여기서 중복 처리하지 않는다.
	*/
	FindSessions(MaxResults);
	return true;
}

void UFPSOnlineSessionSubsystem::ContinueAutoMatchAfterFind(bool WasSuccessful, const TArray<FFPSOnlineSessionInfo>& Sessions)
{
	if (EFPSSessionAutoMatchStage::Finding != _AutoMatchStage)
	{
		return;
	}

	_AutoMatchCandidates.Reset();
	_AutoMatchCandidateCursor = 0;

	if (true == WasSuccessful)
	{
		// 자리가 남아 있는 Session만 참가 후보로 사용한다.
		for (const FFPSOnlineSessionInfo& Information : Sessions)
		{
			if (INDEX_NONE != Information._ResultIndex &&
				Information._CurrentPlayers < Information._MaxPlayers)
			{
				_AutoMatchCandidates.Add(Information._ResultIndex);
			}
		}
	}

	if (0 == _AutoMatchCandidates.Num())
	{
		UE_LOG(LogFPSOnlineSession, Log, TEXT("AutoMatch: no joinable session found. Hosting instead."));
		StartAutoMatchHost();
		return;
	}

	UE_LOG(LogFPSOnlineSession, Log, TEXT("AutoMatch: %d joinable session(s) found. Trying to join."), _AutoMatchCandidates.Num());

	_AutoMatchStage = EFPSSessionAutoMatchStage::Joining;
	JoinSessionByIndex(_AutoMatchCandidates[_AutoMatchCandidateCursor]);
}

void UFPSOnlineSessionSubsystem::AdvanceAutoMatchAfterJoinFailure(const FString& ErrorMessage)
{
	if (EFPSSessionAutoMatchStage::Joining != _AutoMatchStage)
	{
		return;
	}

	UE_LOG(LogFPSOnlineSession, Warning, TEXT("AutoMatch: join candidate failed. %s"), *ErrorMessage);

	/**
	* 참가 실패 후 Local Session 정리까지 실패하면 ConnectionState가 CleanupFailed로 남는다.
	* 이 상태에서는 새 참가도 생성도 허용되지 않으므로 자동 매치를 중단한다.
	*/
	if (EFPSOnlineConnectionState::None != _ConnectionState)
	{
		FinishAutoMatch(false, false,
			FString::Printf(TEXT("%s Auto match aborted: the local session could not be cleaned up."), *ErrorMessage));
		return;
	}

	++_AutoMatchCandidateCursor;

	if (true == _AutoMatchCandidates.IsValidIndex(_AutoMatchCandidateCursor))
	{
		/**
		* 이 호출이 동기적으로 실패하면 BroadcastJoinCompleted를 통해 이 함수가 다시 호출되어
		* 다음 후보로 넘어간다. 따라서 여기서 반복문을 돌리지 않는다.
		*/
		JoinSessionByIndex(_AutoMatchCandidates[_AutoMatchCandidateCursor]);
		return;
	}

	UE_LOG(LogFPSOnlineSession, Log, TEXT("AutoMatch: every join candidate failed. Hosting instead."));
	StartAutoMatchHost();
}

bool UFPSOnlineSessionSubsystem::StartAutoMatchHost()
{
	_AutoMatchStage = EFPSSessionAutoMatchStage::Hosting;
	_AutoMatchCandidates.Reset();
	_AutoMatchCandidateCursor = 0;

	/**
	* CreateSession은 실패 시 완료 통지를 동기적으로 발생시키고,
	* 그 경로에서 FinishAutoMatch -> ResetAutoMatch가 _AutoMatchHostOptions를 비운다.
	* 인자로 넘긴 참조가 그 사이에 무효화되지 않도록 복사본을 사용한다.
	*/
	const FFPSSessionCreateOptions HostOptions = _AutoMatchHostOptions;
	return CreateSession(HostOptions);
}

void UFPSOnlineSessionSubsystem::FinishAutoMatch(bool WasSuccessful, bool IsHost, const FString& ErrorMessage)
{
	if (EFPSSessionAutoMatchStage::None == _AutoMatchStage)
	{
		return;
	}

	ResetAutoMatch();

	UE_LOG(LogFPSOnlineSession, Log, TEXT("AutoMatch finished. Successful = %d, IsHost = %d, %s"),
		WasSuccessful ? 1 : 0, IsHost ? 1 : 0, *ErrorMessage);

	_OnAutoMatchCompleted.Broadcast(WasSuccessful, IsHost, ErrorMessage);
}

void UFPSOnlineSessionSubsystem::ResetAutoMatch()
{
	_AutoMatchStage = EFPSSessionAutoMatchStage::None;
	_AutoMatchCandidates.Reset();
	_AutoMatchCandidateCursor = 0;
	_AutoMatchHostOptions = FFPSSessionCreateOptions{};
}

bool UFPSOnlineSessionSubsystem::IsAutoMatchInProgress() const
{
	return EFPSSessionAutoMatchStage::None != _AutoMatchStage;
}

#pragma endregion