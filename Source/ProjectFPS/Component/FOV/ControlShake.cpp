// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/FOV/ControlShake.h"
#include "Curves/CurveVector.h"

void UControlShake::Activate(float InDuration, UCurveVector* InCurve, FRotator InShakeMagnitude)
{
	ControlShakeParams.Duration       = InDuration;
	ControlShakeParams.Curve          = InCurve;
	ControlShakeParams.ShakeMagnitude = InShakeMagnitude;
	
	TimeElapsed = 0.f;
	
	bIsActive = IsValid(InCurve);
}

bool UControlShake::UpdateShake(float DeltaTime, FRotator& OutShake)
{
	OutShake = FRotator::ZeroRotator;
	
	if (!bIsActive || !IsValid(ControlShakeParams.Curve))
	{
		return false;
	}
	
	TimeElapsed += DeltaTime;
	
	float CurveTime = 0.f;
	
	if (ControlShakeParams.Duration > 0.f)
	{
		if (TimeElapsed >= ControlShakeParams.Duration)
		{
			Clear();
			
			return false;
		}
		
		// 단발의 반동 커브는 시간축 0~1을 사용한다.
		CurveTime = TimeElapsed / ControlShakeParams.Duration;
	}
	else
	{
		float StartTime = 0.f;
		float EndTime   = 0.f;
		ControlShakeParams.Curve->GetTimeRange(StartTime, EndTime);
		
		const float Length = EndTime - StartTime;
		
		if (Length <= KINDA_SMALL_NUMBER)
		{
			Clear();
			
			return false;
		}
		
		TimeElapsed = FMath::Fmod(TimeElapsed, Length);
		CurveTime   = StartTime + TimeElapsed;
	}
	
	const FVector Value = ControlShakeParams.Curve->GetVectorValue(CurveTime);
	
	OutShake = FRotator(Value.X * ControlShakeParams.ShakeMagnitude.Pitch,
	                      Value.Y * ControlShakeParams.ShakeMagnitude.Yaw,
						 Value.Z * ControlShakeParams.ShakeMagnitude.Roll);	
	return true;
}

void UControlShake::Clear()
{
	bIsActive = false;
	TimeElapsed = 0.f;
	ControlShakeParams = FControlShakeParams();
}
