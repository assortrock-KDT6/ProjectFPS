#pragma once

#include "Animation/AnimCurveTypes.h"

// 같은 포즈 평가 안에서 C++ 조준 계산과 Control Rig 사이로 전달하는 값이다.
// 네트워크 복제나 이전 프레임의 AnimInstance 커브 값을 사용하지 않는다.
namespace WeaponPose
{
	inline const FName Enabled(TEXT("WeaponPose_Enabled"));
	inline const FName ADS(TEXT("WeaponPose_ADS"));
	inline const FName Reset(TEXT("WeaponPose_Reset"));
	inline const FName Retraction(TEXT("WeaponPose_Retraction"));
	inline const FName Target[] = { TEXT("WeaponPose_TargetX"), TEXT("WeaponPose_TargetY"), TEXT("WeaponPose_TargetZ") };
	inline const FName Initial[] = { TEXT("WeaponPose_InitialX"), TEXT("WeaponPose_InitialY"), TEXT("WeaponPose_InitialZ"), TEXT("WeaponPose_InitialW") };
	inline const FName MeshRotation[] = { TEXT("WeaponPose_MeshX"), TEXT("WeaponPose_MeshY"), TEXT("WeaponPose_MeshZ"), TEXT("WeaponPose_MeshW") };
	inline const FName LeftPosition[] = { TEXT("WeaponPose_LeftX"), TEXT("WeaponPose_LeftY"), TEXT("WeaponPose_LeftZ") };
	inline const FName LeftRotation[] = { TEXT("WeaponPose_LeftQX"), TEXT("WeaponPose_LeftQY"), TEXT("WeaponPose_LeftQZ"), TEXT("WeaponPose_LeftQW") };

	inline void Write(FBlendedCurve& Curve, const FName (&Names)[3], const FVector& Value)
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Curve.Set(Names[Index], static_cast<float>(Value[Index]));
		}
	}

	inline void Write(FBlendedCurve& Curve, const FName (&Names)[4], const FQuat& Value)
	{
		Curve.Set(Names[0], static_cast<float>(Value.X));
		Curve.Set(Names[1], static_cast<float>(Value.Y));
		Curve.Set(Names[2], static_cast<float>(Value.Z));
		Curve.Set(Names[3], static_cast<float>(Value.W));
	}

	inline FVector Read(const FBlendedCurve& Curve, const FName (&Names)[3])
	{
		return FVector(Curve.Get(Names[0]), Curve.Get(Names[1]), Curve.Get(Names[2]));
	}
}
