// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FPSOnlineSessionKeys.generated.h"

/**
 * 엔진이 정의한 SETTING_GAMEMODE / SETTING_MAPNAME은 그대로 사용한다.
 * 표시 이름만 전용 키로 광고한다.
 */
#define SETTING_FPS_DISPLAYNAME FName(TEXT("FPSDISPLAYNAME"))


UCLASS()
class PROJECTFPS_API UFPSOnlineSessionKeys : public UObject
{
	GENERATED_BODY()
	
};

