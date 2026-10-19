#include "UI/GamePlay/HitDirectionWidget.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UHitDirectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	_DirectionImages = {_DirectionImage0, _DirectionImage1, _DirectionImage2, _DirectionImage3};
	for (UImage* Image : _DirectionImages)
	{
		if (true == IsValid(Image))
		{
			Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		}
	}
	ClearFeedback();
}

void UHitDirectionWidget::ClearFeedback()
{
	_Victim.Reset();
	for (int32 Index = 0; Index < _MaxDirections; ++Index)
	{
		_Hits[Index] = FHitPulse();
		if (true == _DirectionImages.IsValidIndex(Index) && true == IsValid(_DirectionImages[Index]))
		{
			_DirectionImages[Index]->SetRenderOpacity(0.f);
		}
	}
}

void UHitDirectionWidget::ShowIncomingHit(APawn* Victim, const FVector& IncomingWorldDirection, float Damage, float SourceDistance)
{
	if (false == IsValid(Victim) || false == FMath::IsFinite(Damage) || Damage <= 0.f
		|| true == IncomingWorldDirection.ContainsNaN())
	{
		return;
	}

	const FVector Direction = IncomingWorldDirection.GetSafeNormal2D();
	if (true == Direction.IsNearlyZero())
	{
		return;
	}

	if (_Victim.Get() != Victim)
	{
		ClearFeedback();
	}

	_Victim = Victim;

	int32 Selected = INDEX_NONE;
	for (int32 Index = 0; Index < _MaxDirections; ++Index)
	{
		if (_Hits[Index]._Strength > 0.f && FVector::DotProduct(_Hits[Index]._Direction, Direction) > 0.94f)
		{
			// 같은 방향의 연속 총격은 기존 표시의 유지 시간만 갱신한다.
			Selected = Index;
			break;
		}
	}

	if (Selected == INDEX_NONE)
	{
		Selected = 0;
		for (int32 Index = 0; Index < _MaxDirections; ++Index)
		{
			if (_Hits[Index]._Strength <= 0.f)
			{
				Selected = Index;
				break;
			}
			if (_Hits[Index]._Age > _Hits[Selected]._Age)
			{
				Selected = Index;
			}
		}
	}
	float Strength = FMath::Clamp(0.35f + Damage / 60.f, 0.35f, 1.f);
	if (true == FMath::IsFinite(SourceDistance) && SourceDistance >= 0.f
		&& true == FMath::IsFinite(_DistanceThreshold) && SourceDistance >= FMath::Max(0.f, _DistanceThreshold)
		&& true == FMath::IsFinite(_MinDirectionOpacity))
	{
		// BP 값이 높더라도 원거리 표시가 기존 피격 표시보다 더 진해지지는 않게 한다.
		Strength = FMath::Min(Strength, FMath::Clamp(_MinDirectionOpacity, 0.f, 1.f));
	}
	_Hits[Selected] = {Direction, _DirectionDuration * 0.08f, Strength};
}

void UHitDirectionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	APlayerController* Controller = GetOwningPlayer();
	if (false == _Victim.IsValid() || false == IsValid(Controller) || Controller->GetPawn() != _Victim.Get())
	{
		ClearFeedback();
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;

	Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);
	for (int32 Index = 0; Index < _MaxDirections; ++Index)
	{
		FHitPulse& Hit = _Hits[Index];
		Hit._Age += FMath::Max(0.f, InDeltaTime);

		if (Hit._Age >= _DirectionDuration)
		{
			Hit._Strength = 0.f;
		}

		float Angle = 0.f;
		const bool HasDirection = CalculateAngle(Hit._Direction, CameraRotation.Yaw, Angle);

		if (true == _DirectionImages.IsValidIndex(Index) && true == IsValid(_DirectionImages[Index]))
		{
			_DirectionImages[Index]->SetRenderTransformAngle(Angle);
			_DirectionImages[Index]->SetRenderOpacity(true == HasDirection ? Hit._Strength * CalculateOpacity(Hit._Age) : 0.f);
		}
	}
}

bool UHitDirectionWidget::CalculateAngle(const FVector& Direction, float CameraYawDegrees, float& OutAngle) const
{
	OutAngle = 0.f;
	if (true == Direction.ContainsNaN() || true == Direction.IsNearlyZero() || false == FMath::IsFinite(CameraYawDegrees))
	{
		return false;
	}

	// 카메라의 수평 회전을 뺀 뒤 정면 0도, 오른쪽 90도로 UMG 이미지를 회전한다.
	OutAngle = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)) - CameraYawDegrees);
	return true;
}

float UHitDirectionWidget::CalculateOpacity(float Age) const
{
	if (false == FMath::IsFinite(Age) || false == FMath::IsFinite(_DirectionDuration)
		|| _DirectionDuration <= 0.f || Age < 0.f || Age >= _DirectionDuration)
	{
		return 0.f;
	}

	const float T = Age / _DirectionDuration;
	if (T < 0.08f)
	{
		return T / 0.08f;
	}
	return 1.f - FMath::SmoothStep(0.22f, 1.f, T);
}
