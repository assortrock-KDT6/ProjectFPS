#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSDeathCameraActor.generated.h"

class ACharacterPlayer;
class UCameraComponent;

// 소유 플레이어의 클라이언트에만 생성되는 사망 연출 카메라.
UCLASS(Blueprintable)
class PROJECTFPS_API AFPSDeathCameraActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSDeathCameraActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeDeathCamera(ACharacterPlayer* DeadCharacter, double DeathEndServerTime,
		const FVector& FallbackTargetLocation, const FRotator& CameraRotation);

	float GetBlendTime() const { return FMath::Max(0.f, _BlendTime); }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death Camera")
	TObjectPtr<UCameraComponent> _CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera", meta = (ClampMin = "0.0"))
	float _Distance = 280.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera", meta = (ClampMin = "0.0"))
	float _Height = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera", meta = (ClampMin = "0.0"))
	float _AdditionalRetreatDistance = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera", meta = (ClampMin = "0.0"))
	float _BlendTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera")
	FName _FocusSocket = TEXT("pelvis");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera")
	float _FocusHeight = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Camera", meta = (ClampMin = "1.0"))
	float _CollisionProbeRadius = 12.f;

private:
	TWeakObjectPtr<ACharacterPlayer> _DeadCharacter;
	FVector _LastTargetLocation = FVector::ZeroVector;
	FVector _RetreatDirection = -FVector::ForwardVector;
	double _StartServerTime = 0.0;
	double _EndServerTime = 0.0;
	bool _Initialized = false;

	double GetServerTime() const;
	void UpdateCamera();
};
