// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Settings/SettingWidget.h"
#include "Settings/FPSSettingsSubsystem.h"
#include "Settings/FPSGameUserSettings.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "UI/MainHUD.h"
#include "InputCoreTypes.h"

void USettingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 콤보 박스 항목은 한버만 채움 (열 때마다 선택만 다시 맞춤)
	InitOptions();
	
	CB_Resolution->OnSelectionChanged.AddDynamic(this, &USettingWidget::HandleResolutionChanged);
	CB_WindowMode->OnSelectionChanged.AddDynamic(this, &USettingWidget::HandleWindowModeChanged);
	CB_Quality->OnSelectionChanged.AddDynamic(this, &USettingWidget::HandleQualityChanged);
	CB_MinimapMode->OnSelectionChanged.AddDynamic(this, &USettingWidget::HandleMinimapModeChanged);

	SL_MouseSensitivity->OnValueChanged.AddDynamic(this, &USettingWidget::HandleMouseSensitivityChanged);
	SL_MasterVolume->OnValueChanged.AddDynamic(this, &USettingWidget::HandleMasterVolumeChanged);
	SL_SFXVolume->OnValueChanged.AddDynamic(this, &USettingWidget::HandleSFXVolumeChanged);
	SL_MusicVolume->OnValueChanged.AddDynamic(this, &USettingWidget::HandleMusicVolumeChanged);

	BTN_Default->OnClicked.AddDynamic(this, &USettingWidget::HandleDefaultClicked);
	BTN_Cancel->OnClicked.AddDynamic(this, &USettingWidget::HandleCancelClicked);
	BTN_Apply->OnClicked.AddDynamic(this, &USettingWidget::HandleApplyClicked);

	

}

void USettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 열릴 때는 저장된 값으로 시작한다. (적용하지 않고 닫은 변경은 버림.)
	if(UFPSSettingsSubsystem* Subsystem = GetSettingsSubsystem())
	{
		Subsystem->RevertChanges();
	}
	RefreshFromSettings();
}

FReply USettingWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsInViewport() && InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleCancelClicked();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void USettingWidget::InitOptions()
{
	// 해상도 : 모니터가 지원하는 전체 화면 해상도.
	_Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(_Resolutions);

	CB_Resolution->ClearOptions();
	for (const FIntPoint& Resolution : _Resolutions)
	{
		CB_Resolution->AddOption(FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y));
	}
	
	// 창모드 : EWindowMode 순서(0 전체화면, 1 창 전체 화면, 창)
	CB_WindowMode->ClearOptions();
	CB_WindowMode->AddOption(TEXT("전체 화면"));
	CB_WindowMode->AddOption(TEXT("창 전체 화면"));
	CB_WindowMode->AddOption(TEXT("창 모드"));

	// 그래픽 품질 : 스케일러빌리티 레벨 순서 (0 낮음 ~ 3 매우 높음)
	CB_Quality->ClearOptions();
	CB_Quality->AddOption(TEXT("낮음"));
	CB_Quality->AddOption(TEXT("중간"));
	CB_Quality->AddOption(TEXT("높음"));
	CB_Quality->AddOption(TEXT("매우 높음")); 

	// 미니맵 : 회전형, 고정형
	CB_MinimapMode->ClearOptions();
	CB_MinimapMode->AddOption(TEXT("회전형"));
	CB_MinimapMode->AddOption(TEXT("고정형"));
}

void USettingWidget::RefreshFromSettings()
{
	// 저장된 설정에 맞게 선택 항목과 슬라이더 위치를 맞춘다.
	UFPSGameUserSettings* Settings = GetSettings();
	if (nullptr == Settings)
	{
		return;
	}

	// 지원 목록에 없는 창 크기면 목록 끝에 추가해서 표시
	const FIntPoint Resolution = Settings->GetScreenResolution();
	int32 ResolutionIndex = _Resolutions.IndexOfByKey(Resolution);
	if (INDEX_NONE == ResolutionIndex)
	{
		ResolutionIndex = _Resolutions.Add(Resolution);
		CB_Resolution->AddOption(FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y));
	}
	CB_Resolution->SetSelectedIndex(ResolutionIndex);

	CB_WindowMode->SetSelectedIndex(static_cast<int32>(Settings->GetFullscreenMode()));

	// 항목별로 따로 바꾼 상태(사용자 지정)면 -1이 와서 선택 없음으로 표시된다.
	CB_Quality->SetSelectedIndex(Settings->GetOverallScalabilityLevel());
	if (true == Settings->GetMinimapRotate())
	{
		CB_MinimapMode->SetSelectedIndex(0);
	}
	else
	{
		CB_MinimapMode->SetSelectedIndex(1);
	}

	// SetValue는 OnValueChanged를 부르지 않으므로 텍스트도 직접 갱신
	SL_MouseSensitivity->SetValue(Settings->GetMouseSensitivity());
	SetSensitivityText(Settings->GetMouseSensitivity());

	SL_MasterVolume->SetValue(Settings->GetMasterVolume());
	SetPercentText(TXT_MasterVolume, Settings->GetMasterVolume());

	SL_SFXVolume->SetValue(Settings->GetSFXVolume());
	SetPercentText(TXT_SFXVolume, Settings->GetSFXVolume());

	SL_MusicVolume->SetValue(Settings->GetMusicVolume());
	SetPercentText(TXT_MusicVolume, Settings->GetMusicVolume());

}

UFPSSettingsSubsystem* USettingWidget::GetSettingsSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (nullptr == GameInstance)
	{
		return nullptr;
	}
	return GameInstance->GetSubsystem<UFPSSettingsSubsystem>();
}

UFPSGameUserSettings* USettingWidget::GetSettings() const
{
	return UFPSGameUserSettings::GetFPSGameUserSettings();
}

void USettingWidget::SetPercentText(UTextBlock* Text, float Value)
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;
	Text->SetText(FText::AsPercent(Value, &Options));
}

void USettingWidget::SetSensitivityText(float Value)
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 2;
	Options.MaximumFractionalDigits = 2;
	TXT_MouseSensitivity->SetText(FText::AsNumber(Value, &Options));
}

void USettingWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// 코드에서 SetSelectedIndex로 채울떄는 무시하고 사용자가 고른것만 반영함.
	if (ESelectInfo::Direct == SelectionType)
	{
		return;
	}

	UFPSGameUserSettings* Settings = GetSettings();
	const int32 Index = CB_Resolution->GetSelectedIndex();
	if (nullptr == Settings || false == _Resolutions.IsValidIndex(Index))
	{
		return;
	}
	Settings->SetScreenResolution(_Resolutions[Index]);
}

void USettingWidget::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (ESelectInfo::Direct == SelectionType)
	{
		return;
	}

	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode(CB_WindowMode->GetSelectedIndex()));
	}
}

void USettingWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (ESelectInfo::Direct == SelectionType)
	{
		return;
	}

	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetOverallScalabilityLevel(CB_Quality->GetSelectedIndex());
	}
}

void USettingWidget::HandleMouseSensitivityChanged(float Value)
{
	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetMouseSensitivity(Value);
	}
	SetSensitivityText(Value);
}

void USettingWidget::HandleMasterVolumeChanged(float Value)
{
	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetMasterVolume(Value);
	}
	SetPercentText(TXT_MasterVolume, Value);
}

void USettingWidget::HandleSFXVolumeChanged(float Value)
{
	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetSFXVolume(Value);
	}
	SetPercentText(TXT_SFXVolume, Value);
}

void USettingWidget::HandleMusicVolumeChanged(float Value)
{
	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetMusicVolume(Value);
	}
	SetPercentText(TXT_MusicVolume, Value);
}

void USettingWidget::HandleDefaultClicked()
{
	if (UFPSSettingsSubsystem* Subsystem = GetSettingsSubsystem())
	{
		Subsystem->ResetToDefaults();
	}
	RefreshFromSettings();
}

void USettingWidget::HandleCancelClicked()
{
	if (UFPSSettingsSubsystem* Subsystem = GetSettingsSubsystem())
	{
		Subsystem->RevertChanges();
	}
	RefreshFromSettings();

	// 로비에 내장된 설정 패널은 유지하고, HUD가 띄운 설정 창만 닫는다.
	if (IsInViewport())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AMainHUD* HUD = Cast<AMainHUD>(PC->GetHUD()))
			{
				HUD->ToggleSettings();
			}
		}
	}
}

void USettingWidget::HandleApplyClicked()
{
	if (UFPSSettingsSubsystem* Subsystem = GetSettingsSubsystem())
	{
		Subsystem->ApplyAndSave();
	}
}

void USettingWidget::HandleMinimapModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (ESelectInfo::Direct == SelectionType)
	{
		return;
	}

	if (UFPSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetMinimapRotate(0 == CB_MinimapMode->GetSelectedIndex());
	}
}
