#include "UI/GamePlay/DeathmatchTestWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameMode/FPSGameState.h"
#include "GameMode/PlayerStateBase.h"

void UDeathmatchScoreRowWidget::DisplayPlayer(const FPlayerMatchResult& Result, bool IsLocalPlayer, const FText& Status)
{
	// Blueprint reinstancing can invalidate bindings while a refresh is queued.
	if (!IsValid(_RankText) || !IsValid(_PlayerNameText) || !IsValid(_KillScoreText)
		|| !IsValid(_DeathScoreText) || !IsValid(_PlayerStatusText) || !IsValid(_RowBackground))
	{
		return;
	}
	_RankText->SetText(FText::AsNumber(Result._Rank));
	FString PlayerName = Result._PlayerName;
	PlayerName.ReplaceInline(TEXT("\n"), TEXT(" "));
	PlayerName.ReplaceInline(TEXT("\r"), TEXT(" "));
	if (PlayerName.IsEmpty())
	{
		PlayerName = FString::Printf(TEXT("Player %d"), Result._PlayerId);
	}
	_PlayerNameText->SetText(FText::FromString(IsLocalPlayer ? PlayerName + TEXT(" (나)") : PlayerName));
	_KillScoreText->SetText(FText::AsNumber(Result._Stats._KillScore));
	_DeathScoreText->SetText(FText::AsNumber(Result._Stats._DeathScore));
	_PlayerStatusText->SetText(Status);
	_RowBackground->SetBrushColor(IsLocalPlayer ? _LocalRowColor : _OtherRowColor);
}

void UDeathmatchTestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// 입력 모드는 변경하지 않고, 커서가 표시된 경우에만 목록 스크롤을 허용한다.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshDisplay();
	GetWorld()->GetTimerManager().SetTimer(_RefreshTimer, this, &UDeathmatchTestWidget::RefreshDisplay, _RefreshInterval, true);
}

void UDeathmatchTestWidget::NativeDestruct()
{
	if (IsValid(GetWorld()))
	{
		GetWorld()->GetTimerManager().ClearTimer(_RefreshTimer);
	}
	if (IsValid(_ScoreRowsBox))
	{
		_ScoreRowsBox->ClearChildren();
	}
	_ScoreRows.Empty();
	Super::NativeDestruct();
}

void UDeathmatchTestWidget::RefreshDisplay()
{
	if (!IsValid(GetWorld()) || !IsValid(_MatchStatusText) || !IsValid(_LocalStatsText)
		|| !IsValid(_LocalStateText) || !IsValid(_ScoreboardTitleText) || !IsValid(_MatchHintText)
		|| !IsValid(_ScoreRowsBox))
	{
		return;
	}
	const AFPSGameState* GameState = GetWorld()->GetGameState<AFPSGameState>();
	const APlayerController* Controller = GetOwningPlayer();
	const APlayerStateBase* LocalPlayerState = IsValid(Controller) ? Controller->GetPlayerState<APlayerStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		_MatchStatusText->SetText(FText::FromString(TEXT("경기 정보 대기")));
		_LocalStatsText->SetText(FText::FromString(TEXT("KILL  —     DEATH  —")));
		_LocalStateText->SetText(FText::FromString(TEXT("연결 중")));
		_ScoreboardTitleText->SetText(FText::FromString(TEXT("플레이어 대기 중")));
		return;
	}

	const bool MatchEnded = GameState->HasMatchEnded();
	const bool WaitingToStart = !GameState->HasMatchStarted();
	_MatchStatusText->SetText(FText::FromString(MatchEnded ? TEXT("경기 종료")
		: GameState->IsMatchInProgress() ? TEXT("경기 진행 중") : TEXT("시작 대기")));
	_MatchHintText->SetText(FText::FromString(MatchEnded
		? TEXT("최종 결과 · KILL 동점은 공동 순위")
		: TEXT("개인 데스매치 · 사망 후 자동 리스폰")));
	if (WaitingToStart)
	{
		if (GameState->IsStartCountdownActive())
		{
			const int32 StartSeconds = FMath::Max(0, FMath::CeilToInt(GameState->GetRemainingStartDelay()));
			_MatchStatusText->SetText(FText::FromString(FString::Printf(TEXT("전원 도착 · %d초 후 시작"), StartSeconds)));
		}
		else
		{
			_MatchStatusText->SetText(FText::FromString(FString::Printf(TEXT("매치 대기 · 준비 %d / %d명"),
				GameState->GetReadyPlayerCount(), GameState->GetExpectedPlayerCount())));
		}
		_MatchHintText->SetText(FText::FromString(TEXT("시작 전 공격 불가 · 경기 시간은 차감되지 않습니다")));
	}

	if (IsValid(LocalPlayerState))
	{
		// 종료 결과가 도착했다면 개인 표시도 해당 복사본에 맞춘다.
		FPlayerMatchStats LocalStats = LocalPlayerState->GetMatchStats();
		if (MatchEnded)
		{
			for (const FPlayerMatchResult& Result : GameState->GetMatchResults())
			{
				if (Result._PlayerId == LocalPlayerState->GetPlayerId())
				{
					LocalStats = Result._Stats;
					break;
				}
			}
		}
		_LocalStatsText->SetText(FText::FromString(FString::Printf(TEXT("KILL  %d     DEATH  %d"), LocalStats._KillScore, LocalStats._DeathScore)));
		_LocalStateText->SetText(FText::FromString(MatchEnded ? TEXT("경기 종료")
			: LocalPlayerState->IsOnlyASpectator() ? TEXT("관전 중")
			: WaitingToStart ? TEXT("매치 대기 · 공격 불가")
			: LocalPlayerState->IsDead() ? TEXT("리스폰 대기") : TEXT("생존")));
		_LocalStateText->SetColorAndOpacity(FSlateColor(!MatchEnded && LocalPlayerState->IsDead()
			? _DeadStateColor : _ActiveStateColor));
	}
	else
	{
		_LocalStatsText->SetText(FText::FromString(TEXT("KILL  —     DEATH  —")));
		_LocalStateText->SetText(FText::FromString(TEXT("참가자 정보 대기")));
	}
	DisplayScoreboard(GameState, LocalPlayerState);
}

void UDeathmatchTestWidget::BuildLiveResults(const AFPSGameState* GameState, TArray<FPlayerMatchResult>& Results) const
{
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		const APlayerStateBase* FPSPlayerState = Cast<APlayerStateBase>(PlayerState);
		if (!IsValid(FPSPlayerState) || FPSPlayerState->IsOnlyASpectator() || FPSPlayerState->IsInactive())
		{
			continue;
		}
		FPlayerMatchResult& Result = Results.AddDefaulted_GetRef();
		Result._PlayerId = FPSPlayerState->GetPlayerId();
		Result._PlayerName = FPSPlayerState->GetPlayerName();
		Result._Stats = FPSPlayerState->GetMatchStats();
	}
	for (FPlayerMatchResult& Result : Results)
	{
		Result._Rank = 1;
		for (const FPlayerMatchResult& Other : Results)
		{
			if (Other._Stats._KillScore > Result._Stats._KillScore)
			{
				++Result._Rank;
			}
		}
	}
}

bool UDeathmatchTestWidget::ComparePlayerResults(const FPlayerMatchResult& First, const FPlayerMatchResult& Second)
{
	return First._Rank == Second._Rank ? First._PlayerId < Second._PlayerId : First._Rank < Second._Rank;
}

FText UDeathmatchTestWidget::GetPlayerStatus(const AFPSGameState* GameState, int32 PlayerId) const
{
	if (GameState->HasMatchEnded())
	{
		return FText::FromString(TEXT("종료"));
	}
	if (!GameState->HasMatchStarted())
	{
		return FText::FromString(TEXT("대기"));
	}
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		const APlayerStateBase* FPSPlayerState = Cast<APlayerStateBase>(PlayerState);
		if (IsValid(FPSPlayerState) && FPSPlayerState->GetPlayerId() == PlayerId)
		{
			return FText::FromString(FPSPlayerState->IsDead() ? TEXT("대기") : TEXT("생존"));
		}
	}
	return FText::GetEmpty();
}

void UDeathmatchTestWidget::DisplayScoreboard(const AFPSGameState* GameState, const APlayerStateBase* LocalPlayerState)
{
	TArray<FPlayerMatchResult> Results;
	if (GameState->HasMatchEnded())
	{
		Results = GameState->GetMatchResults();
	}
	else
	{
		BuildLiveResults(GameState, Results);
	}
	Results.Sort(&UDeathmatchTestWidget::ComparePlayerResults);
	_ScoreboardTitleText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %d명"),
		GameState->HasMatchEnded() ? TEXT("최종 순위") : TEXT("실시간 순위"), Results.Num())));

	if (!_ScoreRowWidgetClass)
	{
		return;
	}
	for (int32 Index = 0; Index < Results.Num(); ++Index)
	{
		if (!_ScoreRows.IsValidIndex(Index) || !IsValid(_ScoreRows[Index]))
		{
			UDeathmatchScoreRowWidget* Row = CreateWidget<UDeathmatchScoreRowWidget>(GetOwningPlayer(), _ScoreRowWidgetClass);
			if (!IsValid(Row))
			{
				return;
			}
			if (_ScoreRows.IsValidIndex(Index)) _ScoreRows[Index] = Row;
			else _ScoreRows.Add(Row);
			_ScoreRowsBox->AddChildToVerticalBox(Row);
		}
		const FPlayerMatchResult& Result = Results[Index];
		_ScoreRows[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		_ScoreRows[Index]->DisplayPlayer(Result,
			IsValid(LocalPlayerState) && Result._PlayerId == LocalPlayerState->GetPlayerId(), GetPlayerStatus(GameState, Result._PlayerId));
	}
	// 점수 변경마다 위젯을 재생성하지 않고, 재접속/퇴장 때도 기존 행을 재사용한다.
	for (int32 Index = Results.Num(); Index < _ScoreRows.Num(); ++Index)
	{
		if (IsValid(_ScoreRows[Index])) _ScoreRows[Index]->SetVisibility(ESlateVisibility::Collapsed);
	}
}
