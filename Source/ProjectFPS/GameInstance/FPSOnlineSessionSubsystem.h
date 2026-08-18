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
 *		1) CreateSession
 *			- 세션 만들기 (방만들기)
 *		2) FindSession
 *			- 방 찾기
 *		3) JoinSession
 *			- 방 가입하기.
 *		4) DestroySession
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSDestroySessionCompleted, bool, WasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPSOnlineSessionStateChange, EFPSOnlineSessionState, State);

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
	FFPSCreateSessionCompleted		_OnCreateSessionCompleted;
		
	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSFindSessionCompleted		_OnFindSessionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSJoinSessionCompleted		_OnJoinSessionCompleted;
		
	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSDestroySessionCompleted		_OnDestroySessionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Online Session")
	FFPSOnlineSessionStateChange	_OnSessionStateChanged;

private:
	IOnlineSessionPtr				 _SessionInterface;
	TSharedPtr<FOnlineSessionSearch> _SessionSearch;

	bool							_CreateAfterDestoy		  = false;
	int32							_PendingPublicConnections = 0;
	FString							_PendingGameMapPath;
	EFPSOnlineSessionState			_SessionState			  = EFPSOnlineSessionState::Idle;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool CreateSession(int32 PublicConnections, const FString& GameMapPath);

	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool FindSessions(int32 MaxResults = 100);

	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool JoinSessionByIndex(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS|Online Session")
	bool DestroySession();

	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	EFPSOnlineSessionState GetSessionState() const;

	UFUNCTION(BlueprintPure, Category = "FPS|Online Session")
	bool IsBusy();

private:
	// SessionSubsystem 내부에서 돌아가는 멤버함수.
	bool RefreshSessionInterface();
	bool RequireIdle(const TCHAR* Operation, FString& OutError) const;
	bool StartCreateSession();
	bool StartDestroySession();
	
	void SetSessionState(EFPSOnlineSessionState NewState);
	void ClearAllOnlineDelegates();
	void ResetPendingCreate();
	void TravelHostToGame();
	void TravelClientToSession(FName SessionName);

	void HandleCreateSessionComplete(FName SessionName, bool WasSuccessful);
	void HandleFindSessionComplete(bool WasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool WasSuccessful);
};
