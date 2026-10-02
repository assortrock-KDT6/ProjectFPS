// Fill out your copyright notice in the Description page of Project Settings.
#include "Component/FOV/ControlShakeComponent.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Weapons/WeaponRecoilPattern.h"

// todo : 현재 반동이 너무 이상하다싶을정도로 막무가내라 이거 다시 설계할 필요가 있어요

UControlShakeComponent::UControlShakeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UControlShakeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}
	
	FRotator ShakeSum = FRotator::ZeroRotator;
	
	if (LoopingShake)
	{
		FRotator Value = FRotator::ZeroRotator;
		
		if (LoopingShake->UpdateShake(DeltaTime, Value))
		{
			ShakeSum += Value;
		}
		else
		{
			LoopingShake = nullptr;
		}
	}
	
	for (int32 Index = ActiveShakes.Num() -1; Index >= 0; --Index)
	{
		FRotator Value = FRotator::ZeroRotator;
		
		if (ActiveShakes[Index] && ActiveShakes[Index]->UpdateShake(DeltaTime, Value))
		{
			ShakeSum += Value;
		}
		else
		{
			ActiveShakes.RemoveAtSwap(Index);
		}
	}
	
	// 전체 반동값을 매 Tick 프레임에 더하면 과도하게 누적되니까 이전 프레임에서 알려진 양만 입력으로 전달
	DeltaShake		= ShakeSum - ShakeSumPreview;
	ShakeSumPreview = ShakeSum;
	
	Character->AddControllerPitchInput(static_cast<float>(DeltaShake.Pitch));
	Character->AddControllerYawInput(static_cast<float>(DeltaShake.Yaw));
}

void UControlShakeComponent::AddShake(FControlShakeParams Params, bool bInLoop)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	
	if (!Character || !Character->IsLocallyControlled() || !IsValid(Params.Curve))
	{
		return;
	}
	
	if (bInLoop || Params.Duration <= 0.f)
	{
		if (LoopingShake &&
			LoopingShake -> ControlShakeParams.Curve == Params.Curve &&
			LoopingShake -> ControlShakeParams.ShakeMagnitude.Equals(Params.ShakeMagnitude))
		{
			return;
		}
		
		ClearLoopingShake();
		
		LoopingShake = NewObject<UControlShake>(this);
		LoopingShake -> Activate(-1.f, Params.Curve, Params.ShakeMagnitude);
		
		return;
	}
	
	UControlShake* Shake = NewObject<UControlShake>(this);
	Shake->Activate(Params.Duration, Params.Curve, Params.ShakeMagnitude);
	
	ActiveShakes.Add(Shake);
}

void UControlShakeComponent::WeaponFired(FName WeaponID)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	
	if (!Character || (!Character->HasAuthority() && !Character->IsLocallyControlled()) || !IsValid(RecoilPatternData) || !GetWorld())
	{
		return;
	}
	
	const FWeaponRecoilInfo* Information = RecoilPatternData->Data.Find(WeaponID);
	
	if (!Information || !IsValid(Information->PatternSequence) || !IsValid(Information->SingleRecoilCurve) || Information->SingleRecoilDuration <= 0.f)
	{
		return;
	}
		
	int32& Offset = RecoilOffsetMap.FindOrAdd(WeaponID);
	
	// Commit 건영 : PatternSequence와 SingleRecoilCurve 유효성 검사는 위에서 진행해서 중복을 제거
	// 카메라 반동은 이 캐릭터를 직접 조종하는 플레이어에게만 적용하기
	if (Character->IsLocallyControlled() && Information->SingleRecoilDuration > 0.f)
	{
		const FVector Pattern = RecoilPatternData->GetRecoilPatternAt(WeaponID, Offset);
		FControlShakeParams Params;
        Params.Duration = Information->SingleRecoilDuration;
        Params.Curve    = Information->SingleRecoilCurve;
        Params.ShakeMagnitude = FRotator(Pattern.X, Pattern.Y, Pattern.Z);
        
        AddShake(Params);
	}
	
	// 서버는 실제 탄퍼짐, 소유 클라이언트는 화면 표시에 사용
	++Offset;
	
	FTimerHandle& Timer = RecoilOffsetResetTimerMap.FindOrAdd(WeaponID);
	
	GetWorld()->GetTimerManager().ClearTimer(Timer);
	
	if (Information->RecoilOffsetResetTime > 0.f)
	{
		// UObject에 연결된 델리게이트를 사용해서 객체 수명을 추적
		GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateUObject(this, &UControlShakeComponent::ResetRecoilOffset, WeaponID), Information->RecoilOffsetResetTime, false);
	}
	else
	{
		ResetRecoilOffset(WeaponID);
	}
}

void UControlShakeComponent::ClearLoopingShake()
{
	if (LoopingShake)
	{
		LoopingShake -> Clear();
		LoopingShake  = nullptr;
	}
}

void UControlShakeComponent::ResetRecoilOffset(FName WeaponID)
{
	if (int32* Offset = RecoilOffsetMap.Find(WeaponID))
	{
		*Offset = 0;
	}
}

int32 UControlShakeComponent::GetRecoilOffset(FName WeaponID) const
{
	const int32* Offset = RecoilOffsetMap.Find(WeaponID);
	return Offset ? *Offset : 0;
}

void UControlShakeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		for (auto& Entry : RecoilOffsetResetTimerMap)
		{
			GetWorld()->GetTimerManager().ClearTimer(Entry.Value);
		}
	}
	
	RecoilOffsetResetTimerMap.Empty();
	RecoilOffsetMap.Empty();
	ActiveShakes.Empty();
	ClearLoopingShake();
	
	ShakeSumPreview = FRotator::ZeroRotator;
	DeltaShake      = FRotator::ZeroRotator;
	
	Super::EndPlay(EndPlayReason);
}

