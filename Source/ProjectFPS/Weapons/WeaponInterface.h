// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTFPS_API IWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// WeaponID를 받아서 무기 데이터와 능력치 데이터를 초기화한다.
	// initialize 는 언리얼이 호출 경로를 관리하는 함수에요
	// WeaponActor에서 _Implementation은 C++에서 실제 동작을 작성하는 함수입니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	bool InitializeWeapon(FName _WeaponID);
};
