#pragma once

#include "CoreMinimal.h"

namespace WeaponAim
{
	// 시선 원점에서 충돌점까지 80cm 이내이면 근거리 수렴을 끄고,
	// 140cm까지는 연속적으로 복원한다. 실제 발사 목표에는 적용하지 않는다.
	inline double PoseTargetDepth(double Depth)
	{
		const double Weight = FMath::Clamp((Depth - 80.0) / 60.0, 0.0, 1.0);
		const double Blend = Weight * Weight * (3.0 - 2.0 * Weight);
		// 초근접에서는 멀리 있는 시선상의 목표를 사용하여 총과 손의 기본 방향을 유지한다.
		return 1.0 / FMath::Lerp(1.0 / 10000.0, 1.0 / FMath::Max(Depth, 1.0), Blend);
	}

	// 임계 감쇠의 해석해를 사용하여 목표가 바뀐 첫 프레임에도 속도가 갑자기 뛰지 않게 한다.
	// 일정한 목표에서는 프레임을 나누어 평가해도 같은 시간에 같은 결과를 얻는다.
	inline double SmoothTargetValue(double Current, double Target, double& Velocity, double DeltaTime)
	{
		const double Time = FMath::Max(DeltaTime, 0.0);
		const double Rate = 24.0;
		const double Offset = Current - Target;
		const double Step = (Velocity + Rate * Offset) * Time;
		const double Decay = FMath::Exp(-Rate * Time);
		Velocity = (Velocity - Rate * Step) * Decay;
		return Target + (Offset + Step) * Decay;
	}

	// 시점 회전·이동은 현재 프레임을 사용하고, 배경과 캐릭터 경계의 깊이 변화만 완화한다.
	inline FVector SmoothTargetDepth(const FVector& Origin, const FVector& Direction, const FVector& Target,
		double DeltaTime, bool Reset, double& InverseDepth, double& Velocity)
	{
		const FVector Forward = Direction.GetSafeNormal();
		const double Desired = 1.0 / PoseTargetDepth(FVector::DotProduct(Target - Origin, Forward));
		if (true == Reset)
		{
			InverseDepth = Desired;
			Velocity = 0.0;
		}
		else
		{
			InverseDepth = SmoothTargetValue(InverseDepth, Desired, Velocity, DeltaTime);
		}
		return Origin + Forward / FMath::Max(InverseDepth, UE_SMALL_NUMBER);
	}

	// 원본 자세에 더할 회전량만 제한한다. 큰 시점 회전은 기존 Aim Offset이 담당한다.
	inline FQuat LimitRotation(const FQuat& Rotation, double Degrees)
	{
		const double Angle = Rotation.AngularDistance(FQuat::Identity);
		return FQuat::Slerp(FQuat::Identity, Rotation,
			Angle > UE_SMALL_NUMBER ? FMath::Min(FMath::DegreesToRadians(Degrees) / Angle, 1.0) : 1.0).GetNormalized();
	}

	// 손과 총을 함께 이동하여 목표가 총구의 회전 반경 밖에 놓이게 한다.
	// 뒤쪽뿐 아니라 아래쪽으로도 이동할 수 있어 손이 어깨에 겹치는 접힘을 피한다.
	inline float HandRetraction(const FVector& Hand, const FVector& Muzzle, const FVector& Target,
		const FVector& RetreatDirection, float Maximum)
	{
		const FVector ToTarget = Target - Hand;
		const double Radius = FVector::Distance(Muzzle, Hand) + 5.0;
		if (ToTarget.SizeSquared() >= Radius * Radius)
		{
			return 0.f;
		}
		const double Along = FVector::DotProduct(ToTarget, RetreatDirection.GetSafeNormal());
		const double Distance = Along + FMath::Sqrt(FMath::Max(Along * Along + Radius * Radius - ToTarget.SizeSquared(), 0.0));
		return static_cast<float>(FMath::Clamp(Distance, 0.0, static_cast<double>(Maximum)));
	}

	// 정상 패킷 간 회전만 예측한다. 긴 공백이나 순간적인 시점 전환은 외삽하지 않는다.
	inline FRotator AngularVelocity(const FRotator& Previous, const FRotator& Current, double Interval)
	{
		if (Interval <= UE_SMALL_NUMBER || Interval >= 0.25)
		{
			return FRotator::ZeroRotator;
		}
		const FRotator Velocity = (Current - Previous).GetNormalized() * (1.0 / Interval);
		return FMath::Max(FMath::Abs(Velocity.Pitch), FMath::Abs(Velocity.Yaw)) <= 720.0
			? FRotator(Velocity.Pitch, Velocity.Yaw, 0.0) : FRotator::ZeroRotator;
	}

	inline FRotator PredictRotation(const FRotator& Rotation, const FRotator& Velocity, double Age)
	{
		return (Rotation + Velocity * FMath::Clamp(Age, 0.0, 0.05)).GetNormalized();
	}

	// 총구가 근거리 목표를 지나가지 않도록 총열 축을 따라 필요한 후퇴량을 구한다.
	inline float Retraction(const FVector& Pivot, const FVector& Muzzle, const FVector& Barrel, const FVector& Target)
	{
		const FVector Offset = Muzzle - Pivot;
		const FVector Direction = Barrel.GetSafeNormal();
		const double Along = FVector::DotProduct(Offset, Direction);
		const double Radius = FMath::Max(FVector::Distance(Target, Pivot) - 10.0, 0.0);
		const double SideSquared = FMath::Max(Offset.SizeSquared() - Along * Along, 0.0);
		const double Required = Along - FMath::Sqrt(FMath::Max(Radius * Radius - SideSquared, 0.0));
		return static_cast<float>(FMath::Clamp(Required, 0.0, 35.0));
	}

	// 포즈를 회전하면 총구 위치도 이동하므로 상체 척추의 회전 중심을 기준으로 보정량을 구한다.
	// 회전 전 총구에서 목표를 향하는 방향만 맞추면 시차 오차가 남는다.
	inline FQuat Solve(const FVector& Pivot, const FVector& Muzzle, const FVector& Barrel, const FVector& Target)
	{
		const FVector Offset = Muzzle - Pivot;
		const FVector ToTarget = Target - Pivot;
		if (true == Barrel.IsNearlyZero() || true == ToTarget.IsNearlyZero())
		{
			return FQuat::Identity;
		}

		// 회전 중심을 중심으로 하고 목표를 지나는 구와 현재 총구 광선의 교점을 구한다.
		// 이 교점이 목표와 일치하도록 회전하면 총구의 이동까지 반영되어,
		// 반복 보정 없이 근거리에서도 총구 광선이 목표를 정확히 지나게 된다.
		const FVector Direction = Barrel.GetSafeNormal();
		const double Along = FVector::DotProduct(Offset, Direction);
		// 팔이 후퇴하는 전환 중에도 조준을 끄지 않는다. 도달 가능한 최소 반경으로
		// 연속적으로 제한하고, 실제 목표 수렴은 앞선 그립 노드의 후퇴로 확보한다.
		const double RadiusSquared = FMath::Max(ToTarget.SizeSquared(), FMath::Square(Offset.Size() + 1.0));
		const double Distance = -Along + FMath::Sqrt(Along * Along + RadiusSquared - Offset.SizeSquared());

		return FQuat::FindBetweenNormals((Offset + Distance * Direction).GetSafeNormal(), ToTarget.GetSafeNormal());
	}
}
