// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "FPSGameUserSettings.generated.h"

/**
 * 엔진 기본설정(해상도, 창모드, 그래픽 품질) + 게임 전용설정 (마우스 감도, 볼륨) 
 * 각자 PC의 GameUserSettings.ini에 저장되는 로컬 설정이라 서버로 복제 x
 * 
 */
UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class PROJECTFPS_API UFPSGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

protected: 
	// 캐릭터 기본 감도에 곱하는 배율. 1을 기준으로 기본 감도 
	UPROPERTY(Config)
	float _MouseSensitivity = 1.f;

	// 전체 볼륨 (0 ~ 1)
	UPROPERTY(config)
	float _MasterVolume = 1.f;

	// 효과음 볼륨 (0 ~ 1)
	UPROPERTY(Config)
	float _SFXVolume = 1.f;

	// 음악 볼륨 (0 ~ 1)
	UPROPERTY(Config)
	float _MusicVolume = 1.f;

public:
	// 블루프린트와  C++ 어디서든 현재 설정 객체를 가져온다.
	UFUNCTION(BlueprintPure, category = "Settings")
	static UFPSGameUserSettings* GetFPSGameUserSettings();

	// 기본 값으로 버튼에서 엔진 설정과 함께 게임 설정도 초기화한다.
	virtual void SetToDefaults() override;
	
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMouseSensitivity(float Value);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMasterVolume(float Value);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetSFXVolume(float Value);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMusicVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMusicVolume() const;

};
