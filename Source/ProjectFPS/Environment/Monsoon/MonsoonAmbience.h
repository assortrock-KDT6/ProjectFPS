#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsoonAmbience.generated.h"
class UAudioComponent;
class UReverbEffect;
class AMonsoonPreviewDoor;
USTRUCT(BlueprintType)
struct FMonsoonAcousticRoom
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Center = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector HalfExtent = FVector(100.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bWarehouse = false;
};
// Listener-based rain filtering and room reverb, scoped to the new map.
UCLASS(Blueprintable)
class PROJECTFPS_API AMonsoonAmbience : public AActor
{
    GENERATED_BODY()
public:
    AMonsoonAmbience();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Monsoon") TObjectPtr<UAudioComponent> Rain;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monsoon") TArray<FMonsoonAcousticRoom> Rooms;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monsoon") TObjectPtr<UReverbEffect> ConcreteReverb;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monsoon") TObjectPtr<UReverbEffect> WarehouseReverb;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Monsoon") float IndoorMix = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Monsoon") float RainCutoffHz = 18000.f;
private:
    TArray<TWeakObjectPtr<AMonsoonPreviewDoor>> Doors;
    int32 ActiveRoom = INDEX_NONE;
    const FName ReverbTag = TEXT("MonsoonTrainingRoom");
};
