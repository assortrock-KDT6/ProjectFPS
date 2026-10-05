#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MetaHumanOutfitBinderComponent.generated.h"


UCLASS(ClassGroup=(MetaHuman), meta=(BlueprintSpawnableComponent))
class PROJECTFPS_API UMetaHumanOutfitBinderComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMetaHumanOutfitBinderComponent();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MetaHuman Outfit") 
	FName BodyComponentName=TEXT("Body");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MetaHuman Outfit") 
	TArray<FName> OutfitComponentNames;

public:
	UFUNCTION(BlueprintCallable, Category="MetaHuman Outfit") 
	void BindOutfit();
protected:
 virtual void OnRegister() override;

 virtual void BeginPlay() override;
};
