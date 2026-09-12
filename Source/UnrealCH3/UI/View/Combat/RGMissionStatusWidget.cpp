// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Combat/RGMissionStatusWidget.h"
#include "Components/TextBlock.h"

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
			Text_KillObjective->SetText(
				NSLOCTEXT(
					"MissionHUD",
					"UnknownKillTarget",
					"ELIMINATIONS —"
				)
			);
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

	if (!bHasReceivedStatus || bKillObjectiveCompleted != bNewKillCompleted)
	{
		bHasReceiveKillStatus = true;
		bKillObjectiveCompleted = bNewKillCompleted;

		OnKillObjectiveCompletedChanged(bKillObjectiveCompleted);
	}
}
