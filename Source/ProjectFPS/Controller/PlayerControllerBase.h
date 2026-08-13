// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerControllerBase.generated.h"

 /*
  * 
  *	[ 플레이어 컨트롤러 ]
  * 
  *	1) 플레이어 컨트롤러는 플레이어에 빙의하는 존재이다.
  *	2) TeamID를 가지고 있으며, TeamID에 따라 적인지 아군인지 판단한다.
  * 
  * 
  *	[캐릭터 컴포넌트 구조]
  *
  * 1) AbilityComponent		: 캐릭터의 능력치를 관리한다.
  *	2) SkillComponent		: 캐릭터의 스킬을 관리한다. (증강 시스템 / 상인에게 파밍)
  *	3) InventoryComponent	: Item과 Coin이 들어간다.	--> 생각점 : Coin은 Item에 속하는 존재인가?
  *													--> Inventory UI에 시각적으로 보이는 아이템이 아니므로 다른 변수로 나눈다.
  *	3-1) 증강은 상인에게서 코인으로 구매 가능한 아이템이다.
  *	4) 플레이어가 끝까지 들고 있어야하는 데이터는 Controller 쪽으로 옮긴다.	--> Character는 죽으면 Destroy 되지만
  *																		Controller는 플레이어 (조작자)가 살아있으면 계속 유지된다.
  *
  */
UCLASS()
class PROJECTFPS_API APlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	APlayerControllerBase();

protected:
	UPROPERTY(BlueprintReadWrite)
	uint8 _TeamId = 0;

public:
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

};
