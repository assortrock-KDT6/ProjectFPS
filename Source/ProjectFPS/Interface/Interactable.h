// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 가능한 것 의 약솜 컴포넌트가 대상이 뭔지 몰라도 다룰수 있게하는 중간다리.
 */







class PROJECTFPS_API IInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);	// 상호작용은 Interact라는 이름으로 요청하고 누가 요청했는지 전달함.





};
