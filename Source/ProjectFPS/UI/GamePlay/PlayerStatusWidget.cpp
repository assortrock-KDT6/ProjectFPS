// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/PlayerStatusWidget.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/ProgressBar.h"
#include "GameFramework/PlayerController.h"
#include "UI/GamePlay/VitalSegments.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/NativeWidgetHost.h"
#include "UI/GamePlay/SVitalEndCell.h"

/** _
 * _Gauge는 디자이너에서 설정한 브러시 템플릿이자 기존 퍼센트 기반 API로 유지한다.
 * _Segments가 있는 위젯만 고정 크기의 분할 셀 방식을 사용하며,
 * 각 셀이 나타내는 수치는 _VitalSettings.UnitsPerCell 설정값을 따른다.
 */
void UPlayerStatusWidget::RefreshVitalSegments(float Current, float Maximum)
{
	UWidgetTree* Tree = WidgetTree;

	UProgressBar* Template = _Gauge;

	UHorizontalBox* Segments = Tree ? Cast<UHorizontalBox>(Tree->FindWidget(TEXT("_Segments"))) : nullptr;

	if (nullptr == Segments || nullptr == Template)
	{
		return;
	}

	if (_VitalSettings.UnitsPerCell <= 0.f || _VitalSettings.CellSize.X <= 0.f || _VitalSettings.CellSize.Y <= 0.f)
	{
		Segments->SetVisibility(ESlateVisibility::Collapsed);
		Template->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	const int32 Count = _VitalSettings.Count(Maximum);

	if (Segments->GetChildrenCount() != Count)
	{
		Segments->ClearChildren();

		const UImage* Frame = Cast<UImage>(Tree->FindWidget(TEXT("_SegmentFrame")));

		for (int32 Index = 0; Index < Count; ++Index)
		{
			USizeBox* Cell = Tree->ConstructWidget<USizeBox>();

			Cell->SetWidthOverride(_VitalSettings.CellSize.X);

			Cell->SetHeightOverride(_VitalSettings.CellSize.Y);

			if (Index == Count - 1 && Frame && _VitalSettings.EndOuterPoints.Num() >= 3 && _VitalSettings.EndInnerPoints.Num() >= 3)
			{
				UNativeWidgetHost* EndCell = Tree->ConstructWidget<UNativeWidgetHost>();
				EndCell->SetContent(SNew(SVitalEndCell)
					.Settings(_VitalSettings).BarStyle(Template->GetWidgetStyle()).FrameBrush(Frame->GetBrush())
					.FillTint(Template->GetFillColorAndOpacity()));
				Cell->AddChild(EndCell);
				Segments->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(0.f, 0.f, _VitalSettings.CellGap, 0.f));
				continue;
			}

			FWidgetTransform Transform;

			Transform.Shear = FVector2D(_VitalSettings.ShearAngle, 0.f);

			Cell->SetRenderTransform(Transform);

			UOverlay* Layers = Tree->ConstructWidget<UOverlay>();

			Cell->AddChild(Layers);

			UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>();

			Bar->SetWidgetStyle(Template->GetWidgetStyle());

			Bar->SetFillColorAndOpacity(Template->GetFillColorAndOpacity());

			Bar->SetBarFillType(EProgressBarFillType::LeftToRight);

			Bar->SetBarFillStyle(EProgressBarFillStyle::Scale);

			Bar->SetBorderPadding(FVector2D::ZeroVector);

			UOverlaySlot* FillSlot = Layers->AddChildToOverlay(Bar);

			FillSlot->SetHorizontalAlignment(HAlign_Fill);

			FillSlot->SetVerticalAlignment(VAlign_Fill);

			FillSlot->SetPadding(FMargin(_VitalSettings.FillPadding));

			if (nullptr != Frame)
			{
				UImage* Outline = Tree->ConstructWidget<UImage>();

				Outline->SetBrush(Frame->GetBrush());

				UOverlaySlot* FrameSlot = Layers->AddChildToOverlay(Outline);

				FrameSlot->SetHorizontalAlignment(HAlign_Fill);

				FrameSlot->SetVerticalAlignment(VAlign_Fill);
			}
			Segments->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(0.f, 0.f, _VitalSettings.CellGap, 0.f));
		}
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		USizeBox* Cell = CastChecked<USizeBox>(Segments->GetChildAt(Index));

		if (UNativeWidgetHost* EndCell = Cast<UNativeWidgetHost>(Cell->GetChildAt(0)))
		{
			StaticCastSharedPtr<SVitalEndCell>(EndCell->GetContent())->SetPercent(_VitalSettings.Fill(Current, Maximum, Index));

			continue;
		}

		UOverlay* Layers = CastChecked<UOverlay>(Cell->GetChildAt(0));

		CastChecked<UProgressBar>(Layers->GetChildAt(0))->SetPercent(_VitalSettings.Fill(Current, Maximum, Index));
	}

	/** 
	 * 최대 수용량에 맞춰 게이지 배경 Plate의 크기만 늘리고,
	 * 개별 셀의 크기는 늘리지 않는다.
	 */
	if (UImage* Plate = Cast<UImage>(Tree->FindWidget(TEXT("_VitalPlate"))))
	{
		if (UCanvasPanelSlot* PlateSlot = Cast<UCanvasPanelSlot>(Plate->Slot))
		{
			PlateSlot->SetSize(FVector2D(_VitalSettings.PanelExtraWidth + Count * (_VitalSettings.CellSize.X + _VitalSettings.CellGap), _VitalSettings.PanelHeight));
		}
	}

	Template->SetVisibility(ESlateVisibility::Collapsed);

	Segments->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	/** 
	 * EditDefaultsOnly:
	 * 중첩 위젯 템플릿은 이전 레이아웃에 저장된 오래된 설정값이 아니라,
	 * 현재 블루프린트 클래스의 기본 설정값을 사용한다.
	 */

	_VitalSettings = GetClass()->GetDefaultObject<UPlayerStatusWidget>()->_VitalSettings;

	_BoundPlayerController = GetOwningPlayer();

	if (true == _BoundPlayerController.IsValid())
	{
		_BoundPlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UPlayerStatusWidget::HandlePlayerPawnChanged);
	}

	HandlePlayerPawnChanged(nullptr, GetOwningPlayerPawn());
}

void UPlayerStatusWidget::NativeDestruct()
{
	if (true == _BoundPlayerController.IsValid())
	{
		_BoundPlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UPlayerStatusWidget::HandlePlayerPawnChanged);
	}

	_BoundPlayerController.Reset();
	
	RefreshGauge();
	
	_AbilitySystemComponent = nullptr;
	
	Super::NativeDestruct();
}

void UPlayerStatusWidget::HandlePlayerPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(NewPawn);

	InitializeGauge(nullptr != AbilityOwner? AbilityOwner->GetAbilitySystemComponent() : nullptr);
}

void UPlayerStatusWidget::HandleGaugeChanged(const FOnAttributeChangeData& Data)
{
	UpdateGaugeFromAttributes();
}

void UPlayerStatusWidget::UpdateGaugeFromAttributes()
{
	if (false == IsValid(_AbilitySystemComponent) || false == _TargetAttribute.IsValid() || false == _TargetMaxAttribute.IsValid())
	{
		return;
	}

	float CurrentValue	= _AbilitySystemComponent->GetNumericAttribute(_TargetAttribute);
	float MaxValue		= _AbilitySystemComponent->GetNumericAttribute(_TargetMaxAttribute);

	UpdateGauge(MaxValue > 0.f ? FMath::Clamp(CurrentValue / MaxValue, 0.f, 1.f): 0.f);

	RefreshVitalSegments(CurrentValue, MaxValue);

	_OnGaugeChanged.Broadcast(CurrentValue, MaxValue);
}

void UPlayerStatusWidget::UpdateGauge(float Percent)
{
	SetProgressBarUpdate(Percent);
}

void UPlayerStatusWidget::SetProgressBarUpdate(float Percent)
{
	if (nullptr != _Gauge)
	{
		_Gauge->SetPercent(Percent);
	}
}

void UPlayerStatusWidget::InitializeGauge(UAbilitySystemComponent* AbiltySystemComponent)
{
	RefreshGauge();
	_AbilitySystemComponent = AbiltySystemComponent;

	if (nullptr == AbiltySystemComponent)
	{
		SetProgressBarUpdate(0.f);
		RefreshVitalSegments(0.f, 0.f);
		return;
	}

	if (true == _TargetAttribute.IsValid() && true == _TargetMaxAttribute.IsValid())
	{
		_GaugeChangedHandle = _AbilitySystemComponent->
							  GetGameplayAttributeValueChangeDelegate(_TargetAttribute).AddUObject(
							  this, &UPlayerStatusWidget::HandleGaugeChanged);

		_MaxGaugeChangedHandle = _AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(_TargetMaxAttribute)
			.AddUObject(this, &UPlayerStatusWidget::HandleGaugeChanged);
		UpdateGaugeFromAttributes();
	}
}

void UPlayerStatusWidget::RefreshGauge()
{
	if (nullptr != _AbilitySystemComponent)
	{
		if (true == _TargetAttribute.IsValid())
		{
			_AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(_TargetAttribute).Remove(_GaugeChangedHandle);
		}
		if (_TargetMaxAttribute.IsValid())
		{
			_AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(_TargetMaxAttribute).Remove(_MaxGaugeChangedHandle);
		}
	}

	_GaugeChangedHandle.Reset();
	_MaxGaugeChangedHandle.Reset();
}
