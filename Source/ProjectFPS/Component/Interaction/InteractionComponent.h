// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this component's properties
	UInteractionComponent();

public:
	// 상호작용 거리 (임시)
	UPROPERTY(EditAnywhere, Category = "Interact")
	float _InteractDistance = 200.f;

public:
	void TryInteract();
	

		
};
