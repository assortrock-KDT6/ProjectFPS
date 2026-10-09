#include "Title/TitleAtmosphereActor.h"
#include "Engine/RectLight.h"
#include "Engine/StaticMeshActor.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

ATitleAtmosphereActor::ATitleAtmosphereActor()
{
	PrimaryActorTick.bCanEverTick=false;
}

void ATitleAtmosphereActor::BeginPlay()
{
	Super::BeginPlay();
	
	Random.Initialize(RandomSeed);
	
	if (!IsValid(FlickerLight))
	{
		return;
	}
	
	BaseIntensity = FlickerLight->GetLightComponent() ? FlickerLight->GetLightComponent()->Intensity : 0.0f;
	
	if (IsValid(LampMesh))
	{
		LampMaterial = LampMesh->GetStaticMeshComponent()? 
			LampMesh->GetStaticMeshComponent()->CreateDynamicMaterialInstance(EmissiveMaterialSlot) : nullptr;
	}
	
	GetWorldTimerManager().SetTimer(FlickerTimer, this, &ThisClass::BeginBurst, Random.FRandRange(2.5f, 6.8f), false);
}

void ATitleAtmosphereActor::BeginBurst()
{
	RemainingPulses = Random.RandRange(2, 4) * 2;

	bDimPulse = true;

	Pulse();
}

void ATitleAtmosphereActor::Pulse()
{
	if (!IsValid(FlickerLight))
	{
		return;
	}

	if(RemainingPulses--<=0)
	{
		Restore();

		GetWorldTimerManager().SetTimer(FlickerTimer, this, &ThisClass::BeginBurst, Random.FRandRange(4.5f, 9.0f), false);
		return;
	}

	const float Gain = bDimPulse ? Random.FRandRange(0.35f, 0.72f) : 1.0f;
	
	FlickerLight->GetLightComponent()->SetIntensity(BaseIntensity*Gain);

	if (LampMaterial)
	{
		LampMaterial->SetScalarParameterValue(TEXT("TitleFlickerGain"), Gain);
	}

	bDimPulse = !bDimPulse;

	GetWorldTimerManager().SetTimer(FlickerTimer, this, &ThisClass::Pulse, Random.FRandRange(0.075f, 0.17f), false);
}

void ATitleAtmosphereActor::Restore()
{
	if (IsValid(FlickerLight))
	{
		FlickerLight->GetLightComponent()->SetIntensity(BaseIntensity);
	}

	if (LampMaterial)
	{
		LampMaterial->SetScalarParameterValue(TEXT("TitleFlickerGain"), 1);
	}
}

void ATitleAtmosphereActor::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(FlickerTimer);

	Restore();

	Super::EndPlay(Reason);
}
