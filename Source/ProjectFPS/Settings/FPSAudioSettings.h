// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FPSAudioSettings.generated.h"

class USoundMix;
class USoundClass;

/**
* 볼륨 설정에 쓸 에셋 지정. 프로젝트 세팅 -> 게임->FPS Audio 에서 고른다.
* 에셋 경 로를 코드에 박지 않기 위해 분리했다. (DefaultGame.ini 저장)
 * 
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "FPS Audio"))
class PROJECTFPS_API UFPSAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 사용자 볼륨을 적용할 Sound Mix (비어 있는 믹스 하나면됨.)
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundMix> _UserSoundMix;

	// 전체 볼륨을 적용할 최상위 Sound Class (Overall)
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> _MasterSoundClass;

	// 효과음 볼륨을 적용할 Sound Class (SFX)
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> _SFXSoundClass;

	// 음악 볼륨을 적용할 Sound Class (Music)
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> _MusicSoundClass;
};
