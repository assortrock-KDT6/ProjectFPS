// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPSSettingsSubsystem.generated.h"

/**
* 환경 설정 매니저.
* 값은 UFPSGameUserSettings가 보관하고, 적용, 저장, 되돌리기, 초기화는 여기서만 한다.
* 게임 인스턴스와 수명이 같아서 로비와 게임 맵을 오가도 유지함. 로컬전용이라 복제x  
 */

class UFPSGameUserSettings;

UCLASS()
class PROJECTFPS_API UFPSSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:
	FDelegateHandle _WorldInitializedHandle;
	
public:
	
	// UI에서 현재 값을 읽고 바꿀 때 사용하는 설정 값 객체
	UFUNCTION(BlueprintPure, Category = "Settings")
	UFPSGameUserSettings* GetSettings() const;

	// 변경 값을 실제로 적용하고 파일에 저장. (적용버튼)
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAndSave();

	// 저장하지 않은 변경을 버리고 파일에 저장된 값으로 되돌린다. (취소버튼)
	UFUNCTION(BlueprintCallable,  Category = "Settings")
	void RevertChanges();

	// 모든 설정을 기본 값으로 초기화 
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetToDefaults();

	// 저장된 볼륨을 지금 월드의 소리에 적용한다.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAudioSettings();

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	// 맵의 액터가 준비된 직후 호출됨. 
	void HandleWorldInitializedActors(const FActorsInitializedParams& Params);
	void ApplyAudioSettingsToWorld(UWorld* World) const;

};
