// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingWidget.generated.h"

/**
 * 환경 설정 위젯
 * 변경은 설정 객체에만 기록하고, 적용/취소/기본값은 UFPSSettingSubsystem에 맡긴다.
 */

class UComboBoxString;
class USlider;
class UTextBlock;
class UButton;
class UFPSSettingsSubsystem;
class UFPSGameUserSettings;

UCLASS()
class PROJECTFPS_API USettingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 비디오
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> CB_Resolution;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> CB_WindowMode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> CB_Quality;

	// 조작
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_MouseSensitivity;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_MouseSensitivity;

	//사운드
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_MasterVolume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_MasterVolume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_SFXVolume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_SFXVolume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_MusicVolume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_MusicVolume;

	// 하단 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Default;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Cancel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Apply;

private:
	// CB_Resolution 항목과 같은 순서의 해상도 목록
	TArray<FIntPoint> _Resolutions;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	
private:
	// 콤보 박스 항목 채우기
	void InitOptions();

	// 설정 객체의 현재 값을 화면에 표시
	void RefreshFromSettings();
	
	UFPSSettingsSubsystem* GetSettingsSubsystem() const;
	UFPSGameUserSettings* GetSettings() const;

	void SetPercentText(UTextBlock* Text, float Value);
	void SetSensitivityText(float Value);

	UFUNCTION()
	void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleMouseSensitivityChanged(float Value);

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleDefaultClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleApplyClicked();








	
};
