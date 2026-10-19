#include "Environment/Monsoon/MonsoonAmbience.h"
#include "Environment/Monsoon/MonsoonPreviewDoor.h"
#include "Components/AudioComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Sound/ReverbEffect.h"
AMonsoonAmbience::AMonsoonAmbience()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .04f;
    Rain = CreateDefaultSubobject<UAudioComponent>(TEXT("Rain"));
    SetRootComponent(Rain);
    Rain->bAutoActivate = false;
    Rain->bAllowSpatialization = false;
    Rain->bIsUISound = false;
    Rain->SetLowPassFilterEnabled(true);
    Rain->SetVolumeMultiplier(.55f);
}
void AMonsoonAmbience::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { SetActorTickEnabled(false); return; }
    for (TActorIterator<AMonsoonPreviewDoor> It(GetWorld()); It; ++It) Doors.Add(*It);
    Rain->Play();
}
void AMonsoonAmbience::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Camera) return;
    const FVector Listener = Camera->GetCameraLocation();
    int32 RoomIndex = INDEX_NONE;
    for (int32 Index=0; Index<Rooms.Num(); ++Index)
    {
        const auto& Room = Rooms[Index];
        if (FBox(Room.Center-Room.HalfExtent, Room.Center+Room.HalfExtent).IsInsideOrOn(Listener))
        { RoomIndex = Index; break; }
    }
    float TargetMix = RoomIndex == INDEX_NONE ? 0.f : 1.f;
    if (RoomIndex != INDEX_NONE)
    {
        const auto& Room = Rooms[RoomIndex];
        const FBox DoorBounds(Room.Center-Room.HalfExtent-FVector(100.f), Room.Center+Room.HalfExtent+FVector(100.f));
        float Leak = 0.f;
        for (const auto& WeakDoor : Doors)
        {
            const auto* Door = WeakDoor.Get();
            if (Door && DoorBounds.IsInsideOrOn(Door->GetActorLocation()))
                Leak = FMath::Max(Leak, Door->GetOpenFraction() * FMath::Clamp(1.f-FVector::Distance(Listener, Door->GetActorLocation())/500.f, 0.f, 1.f));
        }
        TargetMix -= .5f * Leak;
    }
    IndoorMix = FMath::FInterpTo(IndoorMix, TargetMix, DeltaSeconds, 3.f);
    RainCutoffHz = FMath::Lerp(18000.f, 1200.f, IndoorMix);
    Rain->SetLowPassFilterFrequency(RainCutoffHz);
    Rain->SetVolumeMultiplier(FMath::Lerp(.55f, .18f, IndoorMix));
    if (RoomIndex != ActiveRoom)
    {
        ActiveRoom = RoomIndex;
        UGameplayStatics::DeactivateReverbEffect(this, ReverbTag);
        if (Rooms.IsValidIndex(ActiveRoom))
        {
            auto* Reverb = Rooms[ActiveRoom].bWarehouse ? WarehouseReverb.Get() : ConcreteReverb.Get();
            if (Reverb) UGameplayStatics::ActivateReverbEffect(this, Reverb, ReverbTag, 1.f, .35f, .5f);
        }
    }
}
void AMonsoonAmbience::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetNetMode() != NM_DedicatedServer) UGameplayStatics::DeactivateReverbEffect(this, ReverbTag);
    Super::EndPlay(EndPlayReason);
}
