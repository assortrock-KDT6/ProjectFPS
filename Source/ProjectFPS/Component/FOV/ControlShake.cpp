// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/FOV/ControlShake.h"
#include "Curves/CurveVector.h"

void UControlShake::Activate(const FControlShakeParams& InParams)
{
	// 커브, 시간, 크기와 카메라 적용 여부를 함께 보관
	ControlShakeParams = InParams;
	
	TimeElapsed = 0.f;
	
	bIsActive = IsValid(ControlShakeParams.Curve);
}

bool UControlShake::UpdateShake(float DeltaTime, FRotator& OutShake)
{
	OutShake = FRotator::ZeroRotator;
	
	if (!bIsActive || !IsValid(ControlShakeParams.Curve))
	{
		return false;
	}
	
	const float PreviousTime = TimeElapsed;
	TimeElapsed += DeltaTime;
	
	float CurveTime = 0.f;
	
	if (ControlShakeParams.Duration > 0.f)
	{
		// if (TimeElapsed >= ControlShakeParams.Duration)
		// {
		// 	Clear();
		// 	
		// 	return false;
		// }
		//
		// // 단발의 반동 커브는 시간축 0~1을 사용한다.
		// CurveTime = TimeElapsed / ControlShakeParams.Duration;
		
		// 종료 시간을 넘어도 커브의 마지막 지점은 반드시 계산
		TimeElapsed = FMath::Min(TimeElapsed, ControlShakeParams.Duration);
		CurveTime   = TimeElapsed / ControlShakeParams.Duration;
		
		// 마지막 값을 이번 호출에서 전달한 뒤, 다음 호출부터 종료를 알리기
		bIsActive   = TimeElapsed < ControlShakeParams.Duration;
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
	
	FVector Value = ControlShakeParams.Curve->GetVectorValue(CurveTime);
	
	if (ControlShakeParams.Duration > 0.f && ControlShakeParams.bAffectCamera)
	{
		// 유한 카메라 반동은 이번 프레임에 추가하거나 되돌릴 양만 전달
		Value -= ControlShakeParams.Curve->GetVectorValue(FMath::Clamp(PreviousTime / ControlShakeParams.Duration, 0.f, 1.f));
	}
	
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
