// Fill out your copyright notice in the Description page of Project Settings.
#include "Component/FOV/FPSViewSkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"

UFPSViewSkeletalMeshComponent::UFPSViewSkeletalMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetOnlyOwnerSee(true);
	SetCastShadow(false);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
}

void UFPSViewSkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
}

void UFPSViewSkeletalMeshComponent::Initialize()
{
	TargetHFOV  = FMath::Clamp(DefaultHFOV, 1.f , 179.f);
	CurrentHFOV = TargetHFOV;
	CurrentInterpSpeed = InterpSpeed;
	for (int32 Index = 0; Index < GetNumMaterials(); ++Index)
	{
		if (GetMaterial(Index))
		{
			CreateAndSetMaterialInstanceDynamic(Index);
		}
	}
	UpdateFOV();
}

void UFPSViewSkeletalMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	
	CurrentHFOV = FMath::FInterpTo(CurrentHFOV, TargetHFOV, DeltaTime, CurrentInterpSpeed);
	
	// 창 크기가 변경돼도 현재 화면 비율을 반영하기
	UpdateFOV();
}

void UFPSViewSkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose)
{
	// 이전 총기의 머테리얼이 새 총기에 남지 않도록 ( 같은 무기 Mesh 사용에 색상으로 특징을 준다면 ..  근데 사실 의미 없지만 총기의 마테리얼로 이로치같이 뭐 특성을 준다면 나쁘지않을지도) 
	EmptyOverrideMaterials();
	Super::SetSkeletalMesh(NewMesh, bReinitPose);
	Initialize();
}

void UFPSViewSkeletalMeshComponent::SetTargetHFOV(float InTargetHFOV, float TransientInterpSpeed)
{
	TargetHFOV = FMath::Clamp(InTargetHFOV, 1.f, 179.f);

	CurrentInterpSpeed = TransientInterpSpeed > 0.f ? TransientInterpSpeed : InterpSpeed;
}

void UFPSViewSkeletalMeshComponent::UpdateFOV()
{
	float FinalHFOV = CurrentHFOV;
	UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	
	if (ViewportClient)
	{
		FVector2D ViewportSize = FVector2D::ZeroVector;
		ViewportClient->GetViewportSize(ViewportSize);
		
		if (ViewportSize.X > 0.f && ViewportSize.Y > 0.f)
		{
			// DefaultHFOV 는 16 : 9 화면의 수평 FOV를 기준으로 한다. TODO : 환경설정을 구현할 예정이라면 21 : 9 등 옵션달아도 괜찮을것 같아요
			const float VerticalHalfAngle = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(CurrentHFOV * 0.5f)) * (9.f / 16.f));
			FinalHFOV = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(VerticalHalfAngle) * (ViewportSize.X / ViewportSize.Y)));
		}
	}
	
	for (int32 Index = 0; Index < GetNumMaterials(); ++Index)
	{
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(GetMaterial(Index));
		
		if (Material)
		{
			Material->SetScalarParameterValue(TEXT("FOV"), FinalHFOV);
		}
	}
}
