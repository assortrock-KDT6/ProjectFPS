// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Common/GameDefines.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPSOnlineSessionSubsystem.generated.h"

/**
 *		FPSOnlineSessionSubsystem 의 역할
 *		
 *		0) 첫단계 예외처리 및 로그 열심히 
 * 
 * 
 *		1) CreateSession
 *			- 세션 만들기 (방만들기)
 *		2) FindSession
 *			- 방 찾기
 *		3) JoinSession
 *			- 방 가입하기.
 *		4) LeaveSession
 *			- 방 떠나기
 *		5) DestroySession
 *			- 방 파괴
 */

// Log Category
DECLARE_LOG_CATEGORY_EXTERN(LogFPSOnlineSession, Log, All);

// 전방선언
struct		FFPSOnlineSessionInfo;

// 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSCreateSessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFPSFindSessionCompleted, bool, WasSuccessful, const TArray< FFPSOnlineSessionInfo>&, Sessions, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSJoinSessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSLeaveSessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSDestroySessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineOperationStateChanged, EFPSOnlineOperationState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineConnectionStateChanged, EFPSOnlineConnectionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSSessionTravelFailed, const FString&, ErrorMessage);



enum class EFPSSessionDestroyIntent : uint8
{
	None,
	Recreate,
	JoinRollback,
	Leave,
	Destroy
};

UCLASS()
class PROJECTFPS_API UFPSOnlineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
private:
	// 델리게이트
	FDelegateHandle _CreateSessionCompleteHandle;
	FDelegateHandle _FindSessionCompleteHandle;
	FDelegateHandle _JoinSessionCompleteHandle;
	FDelegateHandle _DestroySessionCompleteHandle;

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
	FFPSSessionTravelFailed				_OnTravelFailed;

private:
	IOnlineSessionPtr				 _SessionInterface;
	TSharedPtr<FOnlineSessionSearch> _SessionSearch;

	int32							_PendingPublicConnections = 0;
	FString							_PendingGameMapPath;
	FString							_PendingJoinError;
	EFPSOnlineOperationState		_OperationState			  = EFPSOnlineOperationState::Idle;
	EFPSOnlineConnectionState		_ConnectionState		  = EFPSOnlineConnectionState::None;
	EFPSSessionDestroyIntent		_DestroyIntent			  = EFPSSessionDestroyIntent::None;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 호스트가 될 플레이어가 Session을 생성한다. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool CreateSession(int32 PublicConnections, const FString& GameMapPath);

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

	/* 온라인 세션 작업이 진행 중이면 true를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	bool IsBusy() const;

private:

	/* IOnlineSessionPtr를 새로고침하고 해당 값이 존재하는지 검증한다. */
	bool RefreshSessionInterface();

	/*  현재 상태가 Idle이면 true를 반환한다. 
		Idle이 아닐 시 fals를 반환하고 해당 원인은 OutError에 적는다. */
	bool RequireIdle(const TCHAR* Operation, FString& OutError) const;

	/*	Session을 만들 수 있는 상태임을 검증하고 모두 통과했을 때 true를 반환한다. 
		CreateSession이 해당 함수를 사용한다. */
	bool StartCreateSession();

	bool StartDestroySession(EFPSSessionDestroyIntent Intent);
	bool CanServerTravel(const FString& MapPath, FString& OutError) const;

	void SetOperationState(EFPSOnlineOperationState NewState);
	void SetConnectionState(EFPSOnlineConnectionState NewState);
		
	void ClearAllOnlineDelegates();
	void ResetPendingCreate();
	bool TravelHostToGame(FString& OutError);
	void TravelClientToSession(FName SessionName);
	void RollbackJoinedSession(const FString& Error);
	void BroadcastDestroyFailure(EFPSSessionDestroyIntent Intent, const FString& Error);

	void HandleCreateSessionComplete(FName SessionName, bool WasSuccessful);
	void HandleFindSessionComplete(bool WasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool WasSuccessful);
};
