#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "MonsoonPreviewDoor.generated.h"
class UStaticMeshComponent;
// Separate prototype; the team can replace these instances with its final door.
UCLASS(Blueprintable)
class PROJECTFPS_API AMonsoonPreviewDoor : public AActor, public IInteractable
{
    GENERATED_BODY()
public:
    AMonsoonPreviewDoor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Monsoon") TObjectPtr<UStaticMeshComponent> DoorLeaf;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monsoon") float OpenAngle = -95.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monsoon", meta=(ClampMin="0.1")) float OpeningSeconds = .7f;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Monsoon") bool bOpen = false;
    UFUNCTION(BlueprintPure, Category="Monsoon") float GetOpenFraction() const { return OpenFraction; }
private:
    float OpenFraction = 0.f;
};
