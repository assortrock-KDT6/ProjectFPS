// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DefaultInput.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UDefaultInput : public UObject
{
	GENERATED_BODY()
	friend class ACharacterPlayer;
public:
	UDefaultInput();
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputMappingContext> _DefaultInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _Jump;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _MouseLook;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _MouseZoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _Parkour;
};
