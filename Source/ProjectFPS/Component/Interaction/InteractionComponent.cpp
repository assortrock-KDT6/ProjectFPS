// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Interaction/InteractionComponent.h"
#include "Interface/Interactable.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"


UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // 하이라이트용
}

void UInteractionComponent::TryInteract() // -> *TraceInteract
{
	AActor* Owner = GetOwner();
	if (nullptr == Owner)
		return;

	// 소유 액터의 카메라 기준 트레이스 
	UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (nullptr == Camera)
		return;
	
	const FVector Start = Camera->GetComponentLocation();

	// 카메라와 캐릭터 거리만큼 사거리 연장(3인칭 보정)
	const float TotalDistance = _InteractDistance + FVector::Dist(Start, Owner->GetActorLocation());
	const FVector End = Start + Camera->GetForwardVector() * TotalDistance;
	
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		// 캐릭터 몸에서 실제 사거리인지 확인 (카메라 거리와 별도)
		if (FVector::Dist(Owner->GetActorLocation(), Hit.ImpactPoint) > _InteractDistance)
			return;

		AActor* HitActor = Hit.GetActor();

		if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
			IInteractable::Execute_Interact(HitActor, Owner);	

		
	}
	
	// 라인 트레이서

	// 스페어 트레이서

}

