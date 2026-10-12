// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "FPSViewSkeletalMeshComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTFPS_API UFPSViewSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()
	
public:
	UFPSViewSkeletalMeshComponent();
	
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose = true) override;
	
	void Initialize();
	
	// InTargetHFOV 가 음수이면 이 컴포넌트의 DefaultHFOV 값으로 복귀
	UFUNCTION(BlueprintCallable, Category = "View Mesh")
	void SetTargetHFOV(float InTargetHFOV, float TransientInterSpeed = -1.f);
	
	void UpdateFOV();

	// 부착된 1인칭 표시물도 팔과 같은 화면 보정 값을 사용한다.
	void SetFOVFollower(UPrimitiveComponent* InFollower) { FOVFollower = InFollower; }
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "View Mesh", meta = (ClampMin = "1.0", ClampMax = "179.0"))
	float DefaultHFOV = 80.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "View Mesh", meta = (ClampMin = "0.0"))
	float InterpSpeed = 10.f;
	
private:
	TWeakObjectPtr<UPrimitiveComponent> FOVFollower;
	float TargetHFOV = 80.f;
	float CurrentHFOV = 80.f;
	float CurrentInterpSpeed = 10.f;
};
