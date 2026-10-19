// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/FPSSettingsSubsystem.h"
#include "Settings/FPSGameUserSettings.h"
#include "Settings/FPSAudioSettings.h"   
#include "Kismet/GameplayStatics.h"      
#include "Sound/SoundMix.h"              
#include "Sound/SoundClass.h"            

UFPSGameUserSettings* UFPSSettingsSubsystem::GetSettings() const
{
	return UFPSGameUserSettings::GetFPSGameUserSettings();
}

void UFPSSettingsSubsystem::ApplyAndSave()
{
	UFPSGameUserSettings* Settings = GetSettings();
	if (nullptr == Settings)
	{
		return;

	}
	// 해상도 
	Settings->ApplySettings(false);

	ApplyAudioSettings();

}

void UFPSSettingsSubsystem::RevertChanges()
{
	UFPSGameUserSettings* Settings = GetSettings();
	if (nullptr == Settings)
	{
		return;
	}

	// 적용 전에 바꾼 값은 아직 저장 전이므로 다시 읽으면 원래대로 복구함.
	Settings->LoadSettings(true);
}

void UFPSSettingsSubsystem::ResetToDefaults()
{
	UFPSGameUserSettings* Settings = GetSettings();
	if (nullptr == Settings)
	{
		return;
	}
	Settings->SetToDefaults();
	ApplyAndSave();
}

void UFPSSettingsSubsystem::ApplyAudioSettings()
{
	ApplyAudioSettingsToWorld(GetWorld());
}

void UFPSSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 게임 시작(로비)과 맵 이동 때마다 저장된 볼륨을 적용한다.
	_WorldInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UFPSSettingsSubsystem::HandleWorldInitializedActors);
}

void UFPSSettingsSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(_WorldInitializedHandle);

	Super::Deinitialize();
}

void UFPSSettingsSubsystem::HandleWorldInitializedActors(const FActorsInitializedParams& Params)
{
	// PIE에서 여러 창을 띄우면 게임 인스턴스가 창마다 따로 있으므로, 내 월드에만 적용한다.
	if (nullptr == Params.World || Params.World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	ApplyAudioSettingsToWorld(Params.World);
}

void UFPSSettingsSubsystem::ApplyAudioSettingsToWorld(UWorld* World) const
{
	const UFPSGameUserSettings* Settings = GetSettings();
	const UFPSAudioSettings* AudioSettings = GetDefault<UFPSAudioSettings>();
	if (nullptr == World || nullptr == Settings || nullptr == AudioSettings)
	{
		return;
	}

	USoundMix* UserSoundMix = AudioSettings->_UserSoundMix.LoadSynchronous();
	if (nullptr == UserSoundMix)
	{
		return;
	}

	// 전체 볼륨은 하위 클래스(SFX, Music)까지 곱해지고, 각 볼륨은 그 클래스에만 곱해진다. (전환 시간 0 = 즉시 적용)
	UGameplayStatics::SetSoundMixClassOverride(World, UserSoundMix, AudioSettings->_MasterSoundClass.LoadSynchronous(), Settings->GetMasterVolume(), 1.f, 0.f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, UserSoundMix, AudioSettings->_SFXSoundClass.LoadSynchronous(), Settings->GetSFXVolume(), 1.f, 0.f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, UserSoundMix, AudioSettings->_MusicSoundClass.LoadSynchronous(), Settings->GetMusicVolume(), 1.f, 0.f, true);

	// 기본 믹스가 아니라 모디파이어로 추가해야 맵을 불러올 때 엔진이 교체하지 않는다.
	UGameplayStatics::PushSoundMixModifier(World, UserSoundMix);

}
