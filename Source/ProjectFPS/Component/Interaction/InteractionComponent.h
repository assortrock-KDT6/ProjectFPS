// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )

class PROJECTFPS_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this component's properties
	UInteractionComponent();

public:
	// 상호작용 거리 (임시)
	UPROPERTY(EditAnywhere, Category = "Interact")
	float _InteractDistance = 250.f;
	
	// 검사하는 선과 구체를 화면에 보여줄지 정하는거 
	// 지역으로선언 못함-> 에디터에서 정보를 알 수 가 없다네.
	UPROPERTY(EditAnywhere, Category = "Interact", meta = (DisplayName = "Draw Debug Interation"))
	bool _bDrawDebug = true;

	// 디버그 선 유지 시간.
	UPROPERTY(EditAnywhere, Category = "Interact", meta = (EditCondition = "_bDrawDebug"))
	float _DebugDrawTime = 0.f;
private:
	// 
	TWeakObjectPtr<AActor> _CurrentTarget;

public:
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target);
	void ServerInteract_Implementation(AActor* Target);
	void PickUpInteract();

protected:
	// 매 프레임 조준 대상 갱신용
	virtual void TickComponent(float DelaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 조준 대상 판정
	AActor* TraceForInteractable() const;
	
	//조준 대상이 변경될 때 정보 패널 갱신
	void UpdateInteractTarget();


		
};
