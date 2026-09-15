// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

/**
 * 플레이어 스테이스만 표현할 때 사용하자고 약속한다.
 * 만약 다른 플레이어의 Status도 표현하고 싶으면 부모 클래스를 하나 만들어서
 * 두 갈래로 나누어야 함.
 * 혹은 블루프린트에서 해당 대상의 Ability 컴포넌트로 따로 설정해줘야한다. -> 맨 처음 시작 할 때
 * 블프 NativeConstruct 쪽에 로직 추가할 것. -> 해당 위젯을 가질 몬스터 클래스의  Widget에 추가해도 됨.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVitalGaugeChangedSignature, float, NewGaugeValue, float, NewMaxGaugeValue);

struct FOnAttributeChangeData;
/**
 * 구조계층 바꿀 확률 큽니다.
 * 우선 테스트로 실행해보겠습니다. (09.04)
 */

UCLASS()
class PROJECTFPS_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly,meta = (AssetRegistrySearchable = "true", BindWidget), Category = "Fps|Status|Gauge")
	TObjectPtr<class UProgressBar> _Gauge;

	UPROPERTY(BlueprintReadWrite, Category = "Fps|Status|Gauge")
	class UAbilitySystemComponent* _AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fps|Status|Attribute")
	FGameplayAttribute _TargetAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fps|Status|Attribute")
	FGameplayAttribute _TargetMaxAttribute;

	UPROPERTY(BlueprintReadWrite, Category = "Fps|Status|Gauge")
	FVitalGaugeChangedSignature	_OnGaugeChanged;

protected:
	FDelegateHandle _GaugeChangedHandle;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	void HandleGaugeChanged(const FOnAttributeChangeData& Data);

	void UpdateGauge(float Percent);
public:
	void SetProgressBarUpdate(float Percent);
	void RefreshGauge();
public:
	UFUNCTION(BlueprintCallable, Category = "FPS|Status|Functions")
	void InitializeGauge(class UAbilitySystemComponent* AbiltySystemComponent);
};
