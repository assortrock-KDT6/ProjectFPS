// Fill out your copyright notice in the Description page of Project Settings.
#include "Weapons/WeaponRecoilPattern.h"
#include "Curves/CurveVector.h"

FVector UWeaponRecoilPattern::GetRecoilPatternAt(FName WeaponID, int32 Offset) const
{
	const FWeaponRecoilInfo* Information = Data.Find(WeaponID);
	
	if (!Information || !IsValid(Information->PatternSequence))
	{
		return FVector::ZeroVector;
	}
	
	float MinTime = 0.f;
	float MaxTime = 0.f;
	Information->PatternSequence->GetTimeRange(MinTime, MaxTime);
	
	const int32 FirstIndex = FMath::CeilToInt(MinTime);
	const int32 LastIndex  = FMath::FloorToInt(MaxTime);
	
	if (LastIndex < FirstIndex)
	{
		return FVector::ZeroVector;
	}
	
	const int32 PatternCount = LastIndex - FirstIndex + 1;
	int32 SampleOffset = FMath::Max(Offset, 0);
	
	if (SampleOffset >= PatternCount)
	{
		const int32 LoopStart = FMath::Clamp(Information->LoopStartOffset,       0, PatternCount - 1);
		const int32 LoopEnd   = FMath::Clamp(Information->LoopEndOffset, LoopStart, PatternCount - 1);
		const int32 LoopCount = LoopEnd - LoopStart + 1;
		SampleOffset		  = LoopStart + (SampleOffset - PatternCount) % LoopCount;
	}
	
	return Information->PatternSequence->GetVectorValue(static_cast<float>(FirstIndex + SampleOffset));
}
