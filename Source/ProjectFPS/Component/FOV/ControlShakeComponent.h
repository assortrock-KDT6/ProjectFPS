// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ControlShake.h"
#include "TimerManager.h"
#include "ControlShakeComponent.generated.h"

class UWeaponRecoilPattern;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTFPS_API UControlShakeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UControlShakeComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void WeaponFired(FName WeaponID);
	
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void AddShake(FControlShakeParams Params, bool bInLoop = false);
	
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void ClearLoopingShake();
	
	UFUNCTION(BlueprintPure, Category = "Recoil")
	int32 GetRecoilOffset(FName WeaponID) const;
	
	UFUNCTION(BlueprintPure, Category = "Recoil")
	FRotator GetDeltaShake() const
	{
		return DeltaShake;
	}
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	TObjectPtr<UWeaponRecoilPattern> RecoilPatternData = nullptr;
	
private:
	UPROPERTY()
	TArray<TObjectPtr<UControlShake>> ActiveShakes;
	
	UPROPERTY()
	TObjectPtr<UControlShake> LoopingShake = nullptr;
	
	TMap<FName, int32> RecoilOffsetMap;
	TMap<FName, FTimerHandle> RecoilOffsetResetTimerMap;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Recoil", meta = (AllowPrivateAccess = "true"))
	FRotator ShakeSumPreview = FRotator::ZeroRotator;
	
	FRotator DeltaShake		 = FRotator::ZeroRotator;
	
	// 카메라에 전달할 변화량을 계산하기 위해 직전 프레임의 카메라 반동 합을 보관한다
	FRotator CameraShakeSum = FRotator::ZeroRotator;
	
	void ResetRecoilOffset(FName WeaponID);
};
