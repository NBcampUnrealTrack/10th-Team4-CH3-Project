// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Combat/RGMissionStatusWidget.h"
#include "Components/TextBlock.h"
#include "Gamemode/RGGameModeBase.h"
#include "Kismet/GameplayStatics.h"

void URGMissionStatusWidget::ApplyMissionStatus(float RemainingSeconds, int32 CurrentKills, int32 RequiredKills)
{
	// 시간 유효성 검사
	if (!FMath::IsFinite(RemainingSeconds))
	{
		return;
	}

	// 음수 방지 및 정수 범위 제한
	const float SafeSeconds = FMath::Clamp(RemainingSeconds, 0.f, 8640000.f);

	//0.5초 -> 00:01 로 변환
	const int32 TotalSeconds = FMath::CeilToInt(SafeSeconds);
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	if (Text_RemainingTime)
	{
		const FString TimeString = FString::Printf(
			TEXT("%02d:%02d"),
			Minutes,
			Seconds
		);

		Text_RemainingTime->SetText(
			FText::FromString(TimeString)
		);
	}

	if (Text_KillObjective)
	{
		if (RequiredKills > 0)
		{
			Text_KillObjective->SetVisibility(ESlateVisibility::Visible);

			const FText KillText = FText::Format(
				NSLOCTEXT(
					"MissionHUD",
					"KillProgress",
					"ELIMINATIONS {0} / {1}"
				),
				FText::AsNumber(FMath::Max(0, CurrentKills)),
				FText::AsNumber(RequiredKills)
			);

			Text_KillObjective->SetText(KillText);
		}
		else
		{
			// 보스맵처럼 RequiredKills=0인 맵에서는 요구 수 자체를 표시하지 않는다.
			Text_KillObjective->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Core 목표는 기존 ApplyMissionStatus 시그니처를 바꾸지 않고
	// GameMode에서 직접 읽는다. 따라서 기존 BP 호출 노드가 깨지지 않는다.
	int32 CurrentCores = 0;
	int32 RequiredCores = 0;

	if (const ARGGameModeBase* GameMode =
		Cast<ARGGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		CurrentCores = GameMode->GetCurrentCoresDestroyed();
		RequiredCores = GameMode->GetRequiredCoresToClear();
	}

	if (Text_CoreObjective)
	{
		if (RequiredCores > 0)
		{
			Text_CoreObjective->SetVisibility(ESlateVisibility::Visible);

			const FText CoreText = FText::Format(
				NSLOCTEXT(
					"MissionHUD",
					"CoreProgress",
					"CORES {0} / {1}"
				),
				FText::AsNumber(FMath::Max(0, CurrentCores)),
				FText::AsNumber(RequiredCores)
			);

			Text_CoreObjective->SetText(CoreText);
		}
		else
		{
			// 보스맵처럼 RequiredCores=0인 맵에서는 요구 수 자체를 표시하지 않는다.
			Text_CoreObjective->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const bool bNewWarningActive = SafeSeconds <= FMath::Max(0.f, TimeWarningThreshold);

	if (!bHasReceivedStatus || bTimeWarningActive != bNewWarningActive)
	{
		bHasReceivedStatus = true;
		bTimeWarningActive = bNewWarningActive;

		OnTimerWarningChanged(bTimeWarningActive);
	}

	// 처치 목표 or 목표 이상 처치했는지 확인
	const bool bNewKillCompleted = RequiredKills > 0 && CurrentKills >= RequiredKills;

	if (!bHasReceiveKillStatus || bKillObjectiveCompleted != bNewKillCompleted)
	{
		bHasReceiveKillStatus = true;
		bKillObjectiveCompleted = bNewKillCompleted;

		OnKillObjectiveCompletedChanged(bKillObjectiveCompleted);
	}

	const bool bNewCoreCompleted =
		RequiredCores > 0 &&
		CurrentCores >= RequiredCores;

	if (!bHasReceivedCoreStatus || bCoreObjectiveCompleted != bNewCoreCompleted)
	{
		bHasReceivedCoreStatus = true;
		bCoreObjectiveCompleted = bNewCoreCompleted;

		OnCoreObjectiveCompletedChanged(bCoreObjectiveCompleted);
	}
}
