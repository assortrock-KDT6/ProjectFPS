// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamePlay/KillLogEntryWidget.h"
#include "GameMode/PlayerMatchStats.h"
#include "GameMode/PlayerStateBase.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Animation/WidgetAnimation.h"


bool UKillLogEntryWidget::SetEntryInformation(const FPlayerKillLogResult& Information)
{
	if (!Information.IsValid() || !_KilledPlayerName || !_KillerPlayerName || !_KillWeaponImage)
	{
		return false;
	}

	_KilledPlayerName->SetText(FText::FromString(Information._KilledPlayerName));
	
	_KillerPlayerName->SetText(FText::FromString(Information._KillerPlayerName));
	
	// ScaleBox가 텍스처의 원본 비율로 아이콘 영역 안에 맞춘다.
	_KillWeaponImage->SetBrushFromTexture(Information._WeaponIcon, true);
	_KillWeaponImage->SetColorAndOpacity(Information._WeaponIconTint);
	_KillWeaponImage->SetVisibility(IsValid(Information._WeaponIcon)
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	const APlayerStateBase* LocalPlayerState = GetOwningPlayerState<APlayerStateBase>();
	_EntryRelation = Information.ResolveRelation(
		IsValid(LocalPlayerState) ? LocalPlayerState->GetPlayerId() : INDEX_NONE,
		IsValid(LocalPlayerState) ? LocalPlayerState->GetTeamId() : INDEX_NONE);

	FLinearColor Color = _UnknownKillColor;
	switch (_EntryRelation)
	{
	case EKillLogRelation::Self: Color = _SelfKillColor; break;
	case EKillLogRelation::Ally: Color = _AllyKillColor; break;
	case EKillLogRelation::Enemy: Color = _EnemyKillColor; break;
	default: break;
	}
	if (_Background)
	{
		_Background->SetBrushColor(Color);
	}
	
	return true;
}

void UKillLogEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (_FadeOut)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UKillLogEntryWidget::HandleFadeOutFinished);
		BindToAnimationFinished(_FadeOut, FinishedEvent);
	}
}

void UKillLogEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	_bExpiring = false;
	SetRenderOpacity(1.0f);

	if (_FadeIn)
	{
		PlayAnimation(_FadeIn);
	}

	if (UWorld* World = GetWorld())
	{
		if (_DisplayDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(_ExpireTimer, this,
				&UKillLogEntryWidget::BeginFadeOut, _DisplayDuration, false);
		}
		else
		{
			// 0초 설정도 다음 프레임에 만료시킨다. SetTimer의 0초는 타이머 해제를 의미한다.
			_ExpireTimer = World->GetTimerManager().SetTimerForNextTick(
				this, &UKillLogEntryWidget::BeginFadeOut);
		}
	}
}

void UKillLogEntryWidget::BeginFadeOut()
{
	if (_bExpiring)
	{
		return;
	}
	_bExpiring = true;

	if (_FadeIn)
	{
		StopAnimation(_FadeIn);
	}

	if (_FadeOut && _FadeOut->GetEndTime() > _FadeOut->GetStartTime())
	{
		PlayAnimation(_FadeOut);
	}
	else
	{
		// 애니메이션이 없어도 로그가 계속 쌓이지 않도록 제거한다.
		RemoveFromParent();
	}
}

void UKillLogEntryWidget::HandleFadeOutFinished()
{
	if (_bExpiring)
	{
		RemoveFromParent();
	}
}

void UKillLogEntryWidget::NativeDestruct()
{
	// 외부에서 먼저 제거한 경우에도 타이머가 다시 호출되지 않게 정리한다.
	_bExpiring = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(_ExpireTimer);
	}

	Super::NativeDestruct();
}
