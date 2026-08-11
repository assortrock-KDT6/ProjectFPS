// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

/**
 * 모든 HUD의 공통베이스 (직접 사용x, 상속 전용)
 * 위젯 생성/제거, 오버레이 토글, CurrentScreen 관리, 입력 모드 전환 담당.
 */

UCLASS()
class PROJECTFPS_API AMainHUD : public AHUD
{
	GENERATED_BODY()

protected:
	

	
};
