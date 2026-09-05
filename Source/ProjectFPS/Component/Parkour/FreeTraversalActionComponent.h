// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Parkour/TraversalActionComponent.h"
#include "FreeTraversalActionComponent.generated.h"

/**
 * Enter Exit만 존재하는게 아닌 지속적인 키입력을 받는 파쿠르 액션은 해당 컴포넌트를 상속 받아서 구현한다.
 * ex ) 벽 등반, 밧줄 이동 등.
 * Enter	-> 몽타주 
 * 지속		-> 애니메이션 블루프린트
 * Exit		-> 몽타주 혹은 애니메이션 종료
 */
UCLASS()
class PROJECTFPS_API UFreeTraversalActionComponent : public UTraversalActionComponent
{
	GENERATED_BODY()
	
};
