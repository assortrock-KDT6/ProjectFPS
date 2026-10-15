#include "Environment/Monsoon/MonsoonPreviewDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
AMonsoonPreviewDoor::AMonsoonPreviewDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Hinge")));
    DoorLeaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeaf"));
    DoorLeaf->SetupAttachment(RootComponent);
    DoorLeaf->SetMobility(EComponentMobility::Movable);
    DoorLeaf->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    DoorLeaf->SetGenerateOverlapEvents(true);
}
void AMonsoonPreviewDoor::Interact_Implementation(AActor* Interactor)
{
    // The project's existing InteractionComponent supplies the server RPC.
    if (HasAuthority() && IsValid(Interactor) && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(350.f))
    { bOpen = !bOpen; ForceNetUpdate(); }
}
void AMonsoonPreviewDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    OpenFraction = FMath::FInterpConstantTo(OpenFraction, bOpen ? 1.f : 0.f, DeltaSeconds, 1.f / FMath::Max(.1f, OpeningSeconds));
    DoorLeaf->SetRelativeRotation(FRotator(0.f, OpenAngle * OpenFraction, 0.f));
}
void AMonsoonPreviewDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMonsoonPreviewDoor, bOpen);
}
