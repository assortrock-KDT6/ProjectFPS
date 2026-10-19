#include "UI/GamePlay/DamageFeedbackWidget.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UDamageFeedbackWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ClearFeedback();
}

void UDamageFeedbackWidget::ClearFeedback()
{
	_Victim.Reset();
	_VignetteAge = _VignetteDuration;
	_VignetteStrength = 0.f;
	if (true == IsValid(_VignetteImage))
	{
		_VignetteImage->SetRenderOpacity(0.f);
	}
}

void UDamageFeedbackWidget::ShowIncomingHit(APawn* Victim, float Damage)
{
	if (false == IsValid(Victim) || false == FMath::IsFinite(Damage) || Damage <= 0.f)
	{
		return;
	}

	if (_Victim.Get() != Victim)
	{
		ClearFeedback();
	}

	_Victim = Victim;
	const float Strength = FMath::Clamp(0.35f + Damage / 60.f, 0.35f, 1.f);
	_VignetteStrength = FMath::Max(Strength, _VignetteAge < _VignetteDuration ? _VignetteStrength : 0.f);
	// 지속 피해 중에는 최대 밝기부터 다시 시작해 테두리가 깜빡이지 않도록 한다.
	_VignetteAge = _VignetteDuration * 0.08f;
	if (true == IsValid(_VignetteImage))
	{
		_VignetteImage->SetRenderOpacity(_VignetteMaxOpacity * _VignetteStrength * CalculateOpacity());
	}
}

void UDamageFeedbackWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const APlayerController* Controller = GetOwningPlayer();
	if (false == _Victim.IsValid() || false == IsValid(Controller) || Controller->GetPawn() != _Victim.Get())
	{
		ClearFeedback();
		return;
	}

	_VignetteAge += FMath::Max(0.f, InDeltaTime);
	if (true == IsValid(_VignetteImage))
	{
		_VignetteImage->SetRenderOpacity(_VignetteMaxOpacity * _VignetteStrength * CalculateOpacity());
	}
}

float UDamageFeedbackWidget::CalculateOpacity() const
{
	if (false == FMath::IsFinite(_VignetteAge) || false == FMath::IsFinite(_VignetteDuration)
		|| _VignetteDuration <= 0.f || _VignetteAge < 0.f || _VignetteAge >= _VignetteDuration)
	{
		return 0.f;
	}

	const float T = _VignetteAge / _VignetteDuration;
	if (T < 0.08f)
	{
		return T / 0.08f;
	}

	// 최대 밝기를 잠깐 유지한 뒤 부드럽게 사라진다.
	return 1.f - FMath::SmoothStep(0.22f, 1.f, T);
}
