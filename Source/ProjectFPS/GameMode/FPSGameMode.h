// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FPSGameMode.generated.h"

/**
 *  [ 부모 클래스 선정 이유 ]
 * 
 * 멀티플레이 슈팅 게임 등 매치 시작/종료 흐름이 필요한 경우
 * GameMode와 GameState를 상속받아서 만든다.
 * 매치 시작/대기/종료 흐름이 필요한 경우: AGameMode + AGameState 조합을 사용
 * 
 */

UCLASS()
class PROJECTFPS_API AFPSGameMode : public AGameMode
{
	GENERATED_BODY()
	
};
