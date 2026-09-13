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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void InitializeWeapon(FName _WeaponID);
};
