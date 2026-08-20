// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/FPSOnlineSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY(LogFPSOnlineSession);

#pragma region FILE_HELPER_FUNCTION

// 언리얼은 Unity 빌드를 실행해서 namespace로 묶어두지 않으면
// 전체 클래스에서 동일한 함수명을 사용하는 함수가 발생시
// 오류가 난다.
namespace
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
}
#pragma endregion


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
		GetGameInstance()->GetTimerManager().ClearTimer(_TravelWatchdogHandle);
	}

	// 델리게이트 해제 전에, 남아있는 세션이 있으면 파괴 요청만 보낸다.
	// 종료 중이므로 비동기 콜백을 기다리지 않는다.
	if (true == _SessionInterface.IsValid() && nullptr != _SessionInterface->GetNamedSession(NAME_GameSession))
	{
		_SessionInterface->DestroySession(NAME_GameSession);
	}

	ClearOnlineSessionDelegates();
	_SessionSearch.Reset();
	_SessionInterface.Reset();

	ResetPendingCreate();

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
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::Joined == _ConnectionState ||
		EFPSOnlineConnectionState::CleanupFailed == _ConnectionState)
	{
		ErrorMessage = TEXT("Clean up the current local session before creating a session.");
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (Options._MaxPlayers <= 0)
	{
		ErrorMessage = TEXT("PublicConnections must be greater than zero.");
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}


	FFPSSessionCreateOptions ValidatedOptions = Options;
	ValidatedOptions._MapId.TrimStartAndEndInline();

	if (false == CanServerTravel(ValidatedOptions._MapId, ErrorMessage))
	{
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	_PendingCreateOptions = MoveTemp(ValidatedOptions);
	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface unavailable.");
		ResetPendingCreate();
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
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

	FString TravelError;
	const bool Started = TravelHostToGame(PendingMapPath, EFPSSessionTravelIntent::Host, TravelError);
	if (false == Started)
	{
		_OnTravelFailed.Broadcast(TravelError);
		_OnMatchStarted.Broadcast(false, TravelError);
		return false;
	}

	// 트래블이 확정된 뒤에 매치 시작을 알린다.
	if (true == _SessionInterface.IsValid())
	{
		if (true == _StartSessionCompleteHandle.IsValid())
		{
			_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(_StartSessionCompleteHandle);
		}
		_StartSessionCompleteHandle.Reset();

		_StartSessionCompleteHandle = _SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
			FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleStartSessionComplete));

		if (false == _SessionInterface->StartSession(NAME_GameSession))
		{
			_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(_StartSessionCompleteHandle);
			_StartSessionCompleteHandle.Reset();

			UE_LOG(LogFPSOnlineSession, Error, TEXT("StartSession request could not be started. Join-in-progress control will not work."));
		}
	}

	return true;
}

bool UFPSOnlineSessionSubsystem::StartCreateSession()
{
	if (false == _SessionInterface.IsValid())
	{
		const FString ErrorMessage = TEXT("Session interface before CreateSession.");
		ResetPendingCreate();
		SetOperationState(EFPSOnlineOperationState::Idle);
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = _PendingCreateOptions._MaxPlayers;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = _PendingCreateOptions._AllowJoinProgress;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bIsLANMatch = IsNullSubsystem(GetWorld());
	Settings.Set(SETTING_MAPNAME, _PendingCreateOptions._MapId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

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
		_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
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

	if (false == WasSuccessful)
	{
		UE_LOG(LogFPSOnlineSession, Error, TEXT("StartSession failed. Session stays in Pending state."));
		return;
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
		_OnCreateSessionCompleted.Broadcast(false, TEXT("Online subsystem failed to create the session."));
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

	_OnCreateSessionCompleted.Broadcast(Started, Started ? FString() : TravelError);
	
	if (false == Started)
	{
		_OnTravelFailed.Broadcast(TravelError);
	}
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
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, ErrorMessage);
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
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, ErrorMessage);
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
		ErrorMessage = TEXT("FindSessions request could not be started.");
		_OnFindSessionCompleted.Broadcast(false, EmptySessions, ErrorMessage);
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
	FString ErrorMessage;

	if (false == RequireIdle(TEXT("JoinSession"), ErrorMessage))
	{
		_OnJoinSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (EFPSOnlineConnectionState::None != _ConnectionState)
	{
		ErrorMessage = TEXT("JoinSession requires a disconnected state.");
		_OnJoinSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (false == _SessionSearch.IsValid() ||
		false == _SessionSearch->SearchResults.IsValidIndex(ResultIndex) ||
		false == _SessionSearch->SearchResults[ResultIndex].IsValid())
	{
		ErrorMessage = TEXT("The selected session result is invalid");
		_OnJoinSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	if (false == RefreshSessionInterface())
	{
		ErrorMessage = TEXT("Online Session interface is unavailable.");
		_OnJoinSessionCompleted.Broadcast(false, ErrorMessage);
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
		_OnJoinSessionCompleted.Broadcast(false, ErrorMessage);
		return false;
	}

	return true;
}
/*
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

	if (true == _SessionInterface.IsValid() && _SessionInterface->GetNamedSession(NAME_GameSession))
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

	_OnJoinSessionCompleted.Broadcast(false, JoinError);
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
	* OnlineSubsystem에서는 Client의 Local NamedSession 정리도 DestorySession API를 통해 수행한다.
	* 여기서 Destory는 Host의 방 자체를 없앤다는 의미가 아니라, 현재 Local User가 가지고 있는 Session 상태를 정리하는 의미로 사용한다.
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

		_OnJoinSessionCompleted.Broadcast(false, JoinError);
	}
	else if (EFPSSessionDestroyIntent::DisconnectRecovery == CompletedIntent)
	{
		SetOperationState(EFPSOnlineOperationState::Idle);
		// 정리가 끝났음을 알려서 리스너가 메인 메뉴로 돌아갈 수 있게 한다.
		_OnLeaveSessionCompleted.Broadcast(true, FString());
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

	if (EFPSSessionTravelIntent::Join == CompletedIntent &&
		NM_Client != LoadedWorld->GetNetMode())
	{
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
		_OnJoinSessionCompleted.Broadcast(true, FString());
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
	if (EFPSOnlineTravelState::Traveling != _TravelState)
	{
		return;
	}

	if (true == IsValid(World) && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const FString TravelError = FString::Printf(TEXT("Travel failure [%d]: %s"), static_cast<int32>(FailureType), *ErrorMessage);
	
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

		StartDestroySession(EFPSSessionDestroyIntent::Destroy);
		_OnTravelFailed.Broadcast(ErrorMessage);
		_OnLobbyReady.Broadcast(false, ErrorMessage);
		return;
	}

	_OnTravelFailed.Broadcast(ErrorMessage);
	_OnMatchStarted.Broadcast(false, ErrorMessage);

}

void UFPSOnlineSessionSubsystem::HandleTravelTimeout()
{
	if (EFPSOnlineTravelState::Traveling != _TravelState)
	{
		return;
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

void UFPSOnlineSessionSubsystem::BeginDisconnectRecovery(const FString& ErrorMessage)
{
	if (EFPSOnlineOperationState::Idle != _OperationState)
	{
		UE_LOG(LogFPSOnlineSession, Warning, TEXT("Disconnect recovery deferred: operation in progress"));
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

void UFPSOnlineSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorMessage)
{
	if (true == IsValid(World) && World->GetGameInstance() != GetGameInstance())
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

	if (EFPSOnlineConnectionState::Joined == _ConnectionState)
	{
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
			_OnCreateSessionCompleted.Broadcast(false, ErrorMessage);
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
			_OnJoinSessionCompleted.Broadcast(false, JoinError);
			break;
		}
		case EFPSSessionDestroyIntent::Leave:
		{
			_OnLeaveSessionCompleted.Broadcast(false, ErrorMessage);
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

void UFPSOnlineSessionSubsystem::SetTravelState(EFPSOnlineTravelState NewState)
{
	if (_TravelState == NewState)
	{
		return;
	}
	
	_TravelState = NewState;

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
	}

	_CreateSessionCompleteHandle.Reset();
	_StartSessionCompleteHandle.Reset();
	_FindSessionCompleteHandle.Reset();
	_JoinSessionCompleteHandle.Reset();
	_DestroySessionCompleteHandle.Reset();
}

void UFPSOnlineSessionSubsystem::ResetPendingCreate()
{
	_PendingCreateOptions = FFPSSessionCreateOptions{};
}




