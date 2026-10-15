// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/MinimapWidget.h"
#include "Component/Minimap/MinimapCaptureComponent.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"


void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetupFromPawn(GetOwningPlayerPawn());
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (false == IsValid(_MapImage))
	{
		return;
	}

	APawn* Pawn = GetOwningPlayerPawn();

	// 죽어서 폰이 없으면 이전 화면이 남지 않게 비움.
	if (false == IsValid(Pawn))
	{
		if (_MapImage->GetRenderOpacity() != 0.f)
		{
			SetupFromPawn(nullptr);
		}

		return;
	}

	// 폰이 바뀌었으면 다시 연결.
	if (Pawn != _BoundPawn.Get())
	{
		SetupFromPawn(Pawn);
	}
}

bool UMinimapWidget::SetupFromPawn(APawn* Pawn)
{
	if (false == IsValid(_MapImage))
	{
		return false;
	}

	if(false == IsValid(Pawn))
	{
		// 머티리얼 브러시는 유지해야 리스폰 시 다시 연결할 수 있다.
		if (UMaterialInstanceDynamic* Material = _MapImage->GetDynamicMaterial())
		{
			Material->SetTextureParameterValue(TEXT("MapTexture"), nullptr);
		}
		_MapImage->SetRenderOpacity(0.f);
		_BoundPawn = nullptr;
		return false;
	}

	UMinimapCaptureComponent* Capture = Pawn->FindComponentByClass<UMinimapCaptureComponent>();
	if (nullptr == Capture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Minimap] 캐릭터에 MinimapCaptureComponent가 없습니다."));
		return false;
	}

	UTextureRenderTarget2D* RenderTarget = Capture->GetRenderTarget();
	if (nullptr == RenderTarget)
	{
		return false;		// 아직 초기화 전. 다음 프레임에 다시 시도한다.
	}

	// FinalColorLDR의 알파는 UI 불투명도가 아니다. UI 머티리얼에서 RGB만 사용한다.
	UMaterialInstanceDynamic* Material = _MapImage->GetDynamicMaterial();
	if (nullptr == Material)
	{
		return false;
	}

	Material->SetTextureParameterValue(TEXT("MapTexture"), RenderTarget);
	_MapImage->SetRenderOpacity(1.f);
	_BoundPawn = Pawn;
	return true;


}
