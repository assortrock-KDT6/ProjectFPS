#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TitleAtmosphereActor.generated.h"

class ARectLight;
class AStaticMeshActor;
class UMaterialInstanceDynamic;

/**
 *  TitleAtmosphereActor는 타이틀 화면에서 분위기를 연출하기 위한 Actor입니다.
 *  이 Actor는 FlickerLight와 LampMesh를 사용하여 조명과 메쉬의 발광 효과를 제어합니다.
 *  RandomSeed를 사용하여 랜덤한 깜빡임 효과를 생성하며, BeginBurst, Pulse, Restore 함수를 통해
 *  깜빡임 애니메이션을 구현합니다.
 */

UCLASS()
class PROJECTFPS_API ATitleAtmosphereActor : public AActor
{
	GENERATED_BODY()

public:
	ATitleAtmosphereActor();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Title Atmosphere") 
	TObjectPtr<ARectLight> FlickerLight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Title Atmosphere") 
	TObjectPtr<AStaticMeshActor> LampMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Title Atmosphere") 
	int32 EmissiveMaterialSlot=1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Title Atmosphere") 
	int32 RandomSeed=24617;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LampMaterial;
	
	FRandomStream Random;
	
	FTimerHandle FlickerTimer;
	
	float BaseIntensity = 0;
	
	int32 RemainingPulses = 0;
	
	bool bDimPulse = true;

protected:
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void BeginBurst();
	
	void Pulse();
	
	void Restore();

};
