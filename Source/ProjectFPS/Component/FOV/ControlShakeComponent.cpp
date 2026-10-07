// Fill out your copyright notice in the Description page of Project Settings.
#include "Component/FOV/ControlShakeComponent.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Weapons/WeaponRecoilPattern.h"

// todo : 현재 반동이 너무 이상하다싶을정도로 막무가내라 이거 다시 설계할 필요가 있어요

/* 반동 수정중...
 * 목적: 카메라의 조준 반동과 총·손의 시각적 반동을 독립적으로 계산한다.
 * 사용 위치: 캐릭터의 반동 컴포넌트와 팔 AnimBP의 ArmsRecoil.
 * 실행 흐름: 발사 시 용도별 반동 등록 → 매 프레임 별도 합산
 *            → 카메라는 변화량 적용, 총·손은 현재 합계를 포즈에 적용.
 */

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
	
	// 이번 프레임의 카메라 합계와 총, 손 합계를 각각 계산한다.
	FRotator ShakeSum = FRotator::ZeroRotator;
	ShakeSumPreview   = FRotator::ZeroRotator;
	
	if (LoopingShake)
	{
		FRotator Value = FRotator::ZeroRotator;
		
		if (LoopingShake->UpdateShake(DeltaTime, Value))
		{
			if (LoopingShake->ControlShakeParams.bAffectCamera)
			{
				ShakeSum += Value;
			}
			else
			{
				ShakeSumPreview += Value;
			}
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
			if (ActiveShakes[Index]->ControlShakeParams.bAffectCamera)
			{
				ShakeSum += Value;
			}
			else
			{
				ShakeSumPreview += Value;
			}
		}
		else
		{
			ActiveShakes.RemoveAtSwap(Index);
		}
	}
	
	// 카메라에는 직전 프레임에서 달라진 양만 전달
	DeltaShake		= ShakeSum - CameraShakeSum;
	CameraShakeSum  = ShakeSum;
	
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
		// 같은 커브라도 적용 대상이 다르면 다른 반동으로 취급
		if (LoopingShake &&
			LoopingShake -> ControlShakeParams.Curve == Params.Curve &&
			LoopingShake -> ControlShakeParams.ShakeMagnitude.Equals(Params.ShakeMagnitude) &&
			LoopingShake -> ControlShakeParams.bAffectCamera == Params.bAffectCamera)
		{
			return;
		}
		
		ClearLoopingShake();
		
		Params.Duration = -1.f;
		LoopingShake = NewObject<UControlShake>(this);
		LoopingShake -> Activate(Params);
		return;
	}
	
	UControlShake* Shake = NewObject<UControlShake>(this);
	Shake->Activate(Params);
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
	
	// Commit 건영 : 기존에 검사했던 커브의 검사는 아래에서 카메라용과 무기용으로 따로 했어요
	if (!Information)
	{
		return;
	}
		
	int32& Offset = RecoilOffsetMap.FindOrAdd(WeaponID);
	
	// Commit 건영 : PatternSequence와 SingleRecoilCurve 유효성 검사는 위에서 진행해서 중복을 제거
	// 카메라 반동은 이 캐릭터를 직접 조종하는 플레이어에게만 적용하기
	// if (Character->IsLocallyControlled() && Information->SingleRecoilDuration > 0.f)
	// {
	// 	const FVector Pattern = RecoilPatternData->GetRecoilPatternAt(WeaponID, Offset);
	// 	FControlShakeParams Params;
    //  Params.Duration = Information->SingleRecoilDuration;
    //  Params.Curve    = Information->SingleRecoilCurve;
    //  Params.ShakeMagnitude = FRotator(Pattern.X, Pattern.Y, Pattern.Z);
    //  
    //  AddShake(Params);
	// }
	
	if (Character->IsLocallyControlled())
	{
		FControlShakeParams Params;
		
		// 탄 번호별 패턴은 카메라 반동의 크기를 결정
		if (IsValid(Information->PatternSequence) && IsValid(Information->SingleRecoilCurve) && Information->SingleRecoilDuration > 0.f)
		{
			const FVector Pattern = RecoilPatternData->GetRecoilPatternAt(WeaponID, Offset);
			
			Params.Duration       = Information->SingleRecoilDuration;
			Params.Curve          = Information->SingleRecoilCurve;
			Params.ShakeMagnitude = FRotator(Pattern.X, Pattern.Y, Pattern.Z);
			Params.bAffectCamera  = true;
			
			AddShake(Params);
		}
		
		// 총, 손은 전용 커브를 사용하며 카메라 패턴의 크기를 곱하지 않음
		if (IsValid(Information->WeaponRecoilCurve) && Information->WeaponRecoilDuration > 0.f)
		{
			Params.Duration       = Information->WeaponRecoilDuration;
			Params.Curve          = Information->WeaponRecoilCurve;
			Params.ShakeMagnitude = FRotator(1.f, 1.f, 1.f);
			Params.bAffectCamera  = false;

			AddShake(Params);
		}
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
	CameraShakeSum  = FRotator::ZeroRotator;
	DeltaShake      = FRotator::ZeroRotator;
	
	Super::EndPlay(EndPlayReason);
}

