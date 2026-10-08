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
	
/* 
* ClearWeaponRecoil() 
* 무기전환시 총과 손에 남은 시각적 반동을 제거
* 이 함수는 총기 반동만 제거하고 ShakeSumPreview를 0으로 만듦
* 두 곳에서 호출 : ClientSetViewWeapon_Implementation() 에서 새 무기 데이터를 적용하기 직전에 / OnRep_CurrentGrenade() 에서 로컬 플레이어의 수류탄 장착이 확인된 직후
* ClearLoopingShake() 는 반복 반동만, ResetRecoilOffset() 은 탄 번호만 처리해서 이 역할을 할 수 없음 
*/
	void ClearWeaponRecoil();
	
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
	// 변경 : 반복 카메라 반동의 변화량과 종류 시 복귀량을 계산할 직전 한계를 보관한다
	FRotator CameraShakeSum = FRotator::ZeroRotator;
	
	void ResetRecoilOffset(FName WeaponID);
};
