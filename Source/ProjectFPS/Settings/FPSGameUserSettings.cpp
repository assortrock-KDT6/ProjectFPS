// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/FPSGameUserSettings.h"

UFPSGameUserSettings* UFPSGameUserSettings::GetFPSGameUserSettings()
{
	return Cast<UFPSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UFPSGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	_MouseSensitivity = 1.f;
	_MasterVolume = 1.f;
	_SFXVolume = 1.f;
	_MusicVolume = 1.f;
}

void UFPSGameUserSettings::SetMouseSensitivity(float Value)
{
	// 0이면 시점이 안 움직이므로 최소값을 둔다.
	_MouseSensitivity = FMath::Clamp(Value, 0.1f, 5.f);
}

void UFPSGameUserSettings::SetMasterVolume(float Value)
{
	_MasterVolume = FMath::Clamp(Value, 0.f, 1.f);
}

void UFPSGameUserSettings::SetSFXVolume(float Value)
{

	_SFXVolume = FMath::Clamp(Value, 0.f, 1.f);
}

void UFPSGameUserSettings::SetMusicVolume(float Value)
{
	_MusicVolume = FMath::Clamp(Value, 0.f, 1.f);
}

float UFPSGameUserSettings::GetMouseSensitivity() const
{
	return _MouseSensitivity;
}

float UFPSGameUserSettings::GetMasterVolume() const
{
	return _MasterVolume;
}

float UFPSGameUserSettings::GetSFXVolume() const
{
	return _SFXVolume;
}

float UFPSGameUserSettings::GetMusicVolume() const
{
	return _MusicVolume;
}
