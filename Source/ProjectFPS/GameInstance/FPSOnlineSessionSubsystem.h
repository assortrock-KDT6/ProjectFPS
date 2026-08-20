// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Common/GameDefines.h"
#include "Common/GameDatas.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPSOnlineSessionSubsystem.generated.h"

// Log Category
DECLARE_LOG_CATEGORY_EXTERN(LogFPSOnlineSession, Log, All);

// 전방선언
class		UNetDriver;

// 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSCreateSessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFPSFindSessionCompleted, bool, WasSuccessful, const TArray< FFPSOnlineSessionInfo>&, Sessions, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSJoinSessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSLeaveSessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSDestroySessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineOperationStateChanged, EFPSOnlineOperationState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineConnectionStateChanged, EFPSOnlineConnectionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineTravelStateChanged, EFPSOnlineTravelState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSSessionTravelFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineConnectionLost, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSMatchStarted, bool, WasSucceful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSOnLobbyReady, bool, WasSucceful, const FString&, ErrorMessage);

/*
*	Recreate		: 기존 Session 제거 후 새로운 Session 생성한다.
*	Leave			: Client가 현재 Local Joined Session 상태를 정리한다.
*	Destroy			: Host가 Session을 종료한디.
*	JoinRollback	: JoinSession 성공 이후 ConnectString 해석(TravelClientToSession 멤버함수 내부),
*					  ClientTravel 또는 네트워크 이동 과정에서 실패한 경우 Local Joined Session을 정리한다.
*/
enum class EFPSSessionDestroyIntent : uint8
{
	None,
	Recreate,
	JoinRollback,
	Leave,
	Destroy,
	DisconnectRecovery
};

enum class EFPSSessionTravelIntent : uint8
{
	None,
	HostLobby,
	Host,
	Join
};

/**
* OnlineSubsystem Session의 생성 / 검색 /  참가 / 이탈 / 종료와
* Session 참가 이후 발생하는 World Travel을 관리하는 GameInstanceSubsystem
* 
* [비동기 처리]
* OnlineSubsystem의 Create / Find / Join / Destroy 요청은 비동기로 수행된다.
* 각 public 함수의 bool 반환값은 최종 성공 여부가 아니라 비동기 요청을 정상적으로 시작했는가를 의미한다.
* 
* 최종 결과는 각 _OnXXXCompleted Delegate를 통해 전달된다.
* 
* [상태 관리]
* - OperationState	: 현재 수행 중인 OnlineSubsystem 비동기 작업
* - ConnectionState : 현재 플레이어와 Session의 관계
* - TravelState		: Session 참가/이동 과정에서 World Travel 진행 여부
* 
* 새로운 Session 작업은 Operation 또는 Travel이 진행 중이지 않을 때만 허용한다.
*/

UCLASS()
class PROJECTFPS_API UFPSOnlineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
private:
	/* 
	* OnlineSubsystem에 등록한 비동기 완료 Delegate Handle.
	* 각 요청이 완료되거나 Subsystem이 종료될 때 반드시 해제하여 동일 Callback이 중복 등록되는 것을 방지한다.
	*/
	FDelegateHandle _CreateSessionCompleteHandle;
	FDelegateHandle _FindSessionCompleteHandle;
	FDelegateHandle _JoinSessionCompleteHandle;
	FDelegateHandle _DestroySessionCompleteHandle;
	FDelegateHandle _StartSessionCompleteHandle;

	/*
	* Engine 레벨 Travel / Network 이벤트 Delegate Handle.
	* Initialize에서 등록하고 Deinitialize에서 해제한다.
	*/
	FDelegateHandle _PostLoadMapHandle;
	FDelegateHandle _TravelFailureHandle;
	FDelegateHandle _NetworkFailureHandle;

	FTimerHandle	_TravelWatchdogHandle;

public:
	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSCreateSessionCompleted			_OnCreateSessionCompleted;
		
	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSFindSessionCompleted			_OnFindSessionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSJoinSessionCompleted			_OnJoinSessionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSLeaveSessionCompleted			_OnLeaveSessionCompleted;
		
	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSDestroySessionCompleted			_OnDestroySessionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSOnlineOperationStateChanged		_OnOperationStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSOnlineConnectionStateChanged	_OnConnectionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSOnlineTravelStateChanged		_OnTravelStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSSessionTravelFailed				_OnTravelFailed;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSOnlineConnectionLost			_OnConnectionLost;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSMatchStarted					_OnMatchStarted;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSOnLobbyReady					_OnLobbyReady;
private:
	IOnlineSessionPtr				 _SessionInterface;
	TSharedPtr<FOnlineSessionSearch> _SessionSearch;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Online Session")
	float _TravelTimeoutSeconds = 30.f;


	/*
	* CreateSession 요청 과정에서 유지해야 하는 임시 데이터.
	* 기존 Session이 존재하면 먼저 DestroySession이 비동기로 완료되어야 하므로 최초 CreateSession()의 인자를 지역 변수만으로 유지할 수 없다.
	* Destroy -> Create로 이어지는 비동기 작업 사이에서 값을 보존하기 위해 저장한다.
	*/
	FFPSSessionCreateOptions		_PendingCreateOptions;
	/*
	* Join 이후 Travel 실패 원인을 임시로 보관한다.
	*/
	FString							_PendingJoinError;

	/*
	* 현재 진행 중인 OnlineSubsystem 비동기 작업.
	* Idle / Creating / Finding / Joining / Destroying 등 *지금 무엇을 하고 있는가*를 표현.
	* World 이동 상태는 이 값을 포함하지 않고 TravelState에서 별도로 관리한다.
	*/
	EFPSOnlineOperationState		_OperationState			  = EFPSOnlineOperationState::Idle;

	/*
	* 현재 플레이어와 Session의 관계.
	* OperationState가 *일시적인 작업 과정*이라면 ConnectionState는 *작업 완료 후 지속되는 Session 관계*를 의미한다.
	*/
	EFPSOnlineConnectionState		_ConnectionState		  = EFPSOnlineConnectionState::None;
	
	/*
	* DestroySession 완료 이후 수행할 후속 작업을 구분한다.
	* OnlineSubsystem의 DestroySession callback에는 *왜 Destoy를 요청했는지*에 대한 정보가 전달되지 않는다.
	* 따라서 DestroySession 요청 전에 Intent를 저장해두고 후속 처리를 수행한다.
	*/
	EFPSSessionDestroyIntent		_DestroyIntent			  = EFPSSessionDestroyIntent::None;

	/*
	* 현재 진행 중인  World Travel 상태.
	* 
	* OpeationState는 OnlineSubsystem 요청의 진행 여부를 나타내고, TravelState는 JoinSession 성공 이후 실제 Host World로 이동하는 별도의 Travel 과정을 나타낸다.
	* Traveling 동안에는 새로운 Session 작업을 시작하지 않는다.
	*/
	EFPSOnlineTravelState			_TravelState			  = EFPSOnlineTravelState::None;

	EFPSSessionTravelIntent			_TravelIntent			  = EFPSSessionTravelIntent::None;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 호스트가 될 플레이어가 Session을 생성한다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool CreateSession(const FFPSSessionCreateOptions& Options);

	/* 호스트가 로비 리슨 서버에 만든 세션을 실제 게임 맵으로 이동시킨다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool StartMatch(const FString& MapPath);

	/* 들어올 수 있는 Session을 조회한다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool FindSessions(int32 MaxResults = 100);

	/* 게스트가 Session에 가입한다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool JoinSessionByIndex(int32 ResultIndex);

	/* 게스트가 Session을 떠난다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool LeaveSession();

	/* 호스트가 Session을 파괴한다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool DestroySession();

	/* 현재 Session 작업 상태를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	EFPSOnlineOperationState	GetOperationState() const;

	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	EFPSOnlineConnectionState	GetConnectionState() const;

	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	EFPSOnlineTravelState GetTravelState() const;


	/* 온라인 세션 작업이 진행 중이면 true를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	bool IsBusy() const;

private:

	/* 
	* 현재 World가 사용하는 OnlineSubsystem으로부터 SessionInterface를 다시 얻는다.
	* GameInstanceSubsystem의 수명 동안 World / OSS 환경이 변경될 가능성을 고려하여 Session 작업을 시작하기 전에 Interface가 아직 유효한지 다시 확인한다.
	* 반환값이 false라면 해당 Session 작업을 시작해서는 안된다.
	*/
	bool RefreshSessionInterface();

	/*  
	* Session 비동기 작업의 중복 실행을 막는다.
	* 
	* OnlineSubsystem Callback이 돌아오기 전에 다른 작업을 시작하면 
	* Delegate Handle, Peding Data, OperationState가 서로 덮어써질 수 있으므로
	* 모든 외부 Session 요청은 이 검사를 먼저 통과해야 한다.
	*/
	bool RequireIdle(const TCHAR* Operation, FString& OutErrorMessage) const;

	/*
	* 실제 OnlineSubsystem CreateSession 요청을 시작한다.
	* 
	* 외부에서 직접 호출하지 않는다.
	* CreateSession()이 입력값과 현재 상태를 검증하고 필요한 Pending Data를 준비한 이후에만 호출되어야 한다.
	* 기존 Session 재생성 과정에서는 DestroySession 완료 Callback에서도 호출 될 수 있다.
	*/
	bool StartCreateSession();
	bool StartDestroySession(EFPSSessionDestroyIntent Intent);

	/*
	* ServerTravel 요청 전에 확인 가능한 조건을 사전 검증한다.
	* 이 검사는 명백한 실패(Map 없음, 잘못된 Package Path, Client 호출)를 미리 걸러내기 위한 것이며 ServerTravel 성공 자체를 보장하지 않는다.
	* 최종 결과는 ServerTravel()의 반환값 및 Travel Failure 처리를 통해 확인해야 한다.
	*/
	bool CanServerTravel(const FString& MapPath, FString& OutErrorMessage) const;

	void SetOperationState(EFPSOnlineOperationState NewState);
	void SetConnectionState(EFPSOnlineConnectionState NewState);
	void SetTravelState(EFPSOnlineTravelState NewState);

	/*
	* Subsystem 종료 시 OnlineSubsystem에 등록했던 Session Delegate를 제거한다.
	* UObject가 파괴되는 과정에서 비동기 Callback이 들어오는 것을 방지하고, 
	* 다음 GameInstance에서 동일 Delegate가 중복 등록되는 문제를 예방한다.
	*/
	void ClearOnlineSessionDelegates();

	void ResetPendingCreate();
	bool TravelHostToGame(const  FString& MapPath, EFPSSessionTravelIntent Intent, FString& OutErrorMessage);
	void TravelClientToSession(FName SessionName);

	/*
	* JoinSession은 성공했지만 이후 접속 주소 해석,
	* ClientTravel 또는 Network/Travel 과정에서 실패한 경우
	* OnlineSubsystem에 남아 있는 Local Joined  Session을 정리한다.
	* 
	* Rollback 과정의 DestroySession은 비동기로 완료되므로 원래 실패 원인을 _PendingJoinError에 보존한다.
	*/
	void RollbackJoinedSession(const FString& ErrorMessage);

	void BroadcastDestroyFailure(EFPSSessionDestroyIntent Intent, const FString& ErrorMessage);
	
	void HandleStartSessionComplete(FName SessionName, bool WasSuccessful);
	void HandleCreateSessionComplete(FName SessionName, bool WasSuccessful);
	void HandleFindSessionComplete(bool WasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool WasSuccessful);
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);

	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorMessage);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Error);
	void HandleJoinTravelFailure(const FString& ErrorMessage);

	void HandleHostTravelFailure(EFPSSessionTravelIntent FailedIntent, const FString& ErrorMessage);
	void HandleTravelTimeout();

	void BeginDisconnectRecovery(const FString& ErrorMessage);
};
