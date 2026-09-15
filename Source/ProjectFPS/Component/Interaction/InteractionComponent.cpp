// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Interaction/InteractionComponent.h"
#include "Interface/Interactable.h"
#include "Camera/CameraComponent.h"
#include "Engine/OverlapResult.h"
#include "Item/ItemPickUp.h" // 이건 옮기기 
#include "UI/GameHUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"




UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // 하이라이트용

	SetIsReplicatedByDefault(true);	// 컴포넌트 네트워크 복제를 사용한다.
}

void UInteractionComponent::ServerInteract_Implementation(AActor* Target)
{
	AActor* Owner = GetOwner();
	if (nullptr == Owner || nullptr == Target)
		return;

	// 서버 재검증 (클라 신호를 그대로 믿지 않음)
	if (false == Target->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		return;
	
	if (FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) > _InteractDistance)
		return;
	// 서버에서 실행
	IInteractable::Execute_Interact(Target, Owner); 
}

AActor* UInteractionComponent::TraceForInteractable() const
{
	AActor* Owner = GetOwner();
	if (nullptr == Owner)
		return nullptr;

	// 소유 액터의 카메라 기준 트레이스 
	UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (nullptr == Camera)
		return nullptr;

	const FVector Start = Camera->GetComponentLocation();

	// 카메라와 캐릭터 거리만큼 사거리 연장(3인칭 보정)
	const float TotalDistance = _InteractDistance + FVector::Dist(Start, Owner->GetActorLocation());
	const FVector End = Start + Camera->GetForwardVector() * TotalDistance;

	FHitResult GroundHit;	// 선이 무엇에 어디에 부딪혔는지 저장하는 변수
	FCollisionQueryParams Params;	// 충돌 검사의 추가 조건을 담는 변수
	Params.AddIgnoredActor(Owner);	// 액터중 충돌을 무시 -> 캐릭터 자신을 무시함.

	bool bHit = GetWorld()->LineTraceSingleByChannel(GroundHit, Start, End, ECC_Visibility, Params);
	
	// 디버그 확인용 - 라인
	if (_bDrawDebug)
		DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, _DebugDrawTime, 0, 2.f);

	// 부딪힌 곳이 없으면 종료
	if (false == bHit)
		return nullptr;

	// 충돌 지점 주변을 구체로 검사함. 
	TArray<FOverlapResult> Overlaps;
	const bool bOverlap = GetWorld()->OverlapMultiByObjectType(
		Overlaps, // 겹친 결과들을 저장함.
		GroundHit.ImpactPoint, // 구체 중심 앞서 선이 부딪힌 위치.
		FQuat::Identity,	// 추가 회전 없음
		FCollisionObjectQueryParams::AllObjects,	// 모든 충돌 오브젝트 타입을 검사 대상으로 지정 
		FCollisionShape::MakeSphere(50.f),	//// 반지름의 크기 50
		Params);	// 본인 캐릭터 제외 

	// 디버그 확인용 - 구체
	if (_bDrawDebug)
		DrawDebugSphere(GetWorld(), GroundHit.ImpactPoint, 50.f, 24, FColor::Yellow, false, _DebugDrawTime, 0, 1.f);

	if (false == bOverlap)
		return nullptr;

	// 가장 가까운 대상 선택.
	// 선택 전이니 비워둠.
	AActor* Obj = nullptr;

	// 현재까지 가장 짧은 거리 제곱을 저장함.
	float CloseObj = TNumericLimits<float>::Max();
	
	// 배열에서 결과를 하나씩 확인함. 
	for (const FOverlapResult& Overlap : Overlaps)
	{
		// 충돌된, 이번에 확일할 액터 가져오기.
		AActor* HitActor = Overlap.GetActor();
		// 충돌된게 없다면 넘어감.
		if (nullptr == HitActor)
			continue;

		// 충될된액터가 인터페이스를 사용하는지 확인함. -> 상호작용 용도(아이템, 지형, 문, 로프, 사다리등등?)
		if (false == HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
			continue;

		// 선이 부딪힌 지점과 액터 위치 사이의 거리를 제곱으로 구함 -> 충돌 지점에서 거리 비교 
		// 제곱을 쓰는 이유 -> 거리구하는 공식 생각하면 이해할수있음.
		const float DistSq = FVector::DistSquared(GroundHit.ImpactPoint, HitActor->GetActorLocation());
		if (DistSq < CloseObj)
		{
			CloseObj = DistSq;
			Obj = HitActor;
		}
	}
	return Obj;
}

void UInteractionComponent::UpdateInteractTarget()
{
	// 조준 대상을 확인. 
	AActor* NewTarget = TraceForInteractable();
	AActor* OldTarget = _CurrentTarget.Get();

	// 가리키던 액터 사라졌는지 (습득처리)
	const bool  bOldDestroyed = _CurrentTarget.IsStale();

	// 대상이 그대로면 아무것도 안함.
	if (NewTarget == OldTarget && false == bOldDestroyed)
		return;

	_CurrentTarget = NewTarget;

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Pc = Cast<APlayerController>(Pawn->GetController());

	AGameHUD* HUD = nullptr;
	if (nullptr != Pc)
		HUD = Cast<AGameHUD>(Pc->GetHUD());

	// 조준한 대상이 아이템인지 확인용(GetTID()를 받기 위함-> 옮기면 수정해야함.)
	AItemPickUp* NewItem = Cast<AItemPickUp>(NewTarget);
	if (nullptr == NewItem)
	{
		if (nullptr != HUD)
			HUD->HideItemInfo();

		return;
	}

	if (nullptr != HUD)
		HUD->ShowItemInfo(NewItem->GetTID());

}

void UInteractionComponent::PickUpInteract()
{
	AActor* Target = TraceForInteractable();
	if (nullptr == Target)
		return;
	ServerInteract(Target);
}

void UInteractionComponent::TickComponent(float DelaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DelaTime, TickType, ThisTickFunction);

	// 컴포넌트를 보유하고 있는 액터를 가지고와서 APawn인지 확인함.
	APawn* Pawn = Cast<APawn>(GetOwner());

	// 폰이 비어있거나 조종하는 폰이 아닐경우 반환.
	if (nullptr == Pawn || false == Pawn->IsLocallyControlled())
		return;

	UpdateInteractTarget();
}
