// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Combat/RGQuickSlotWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void URGQuickSlotWidget::ApplyCooldown(float RemaningSeconds, float CooldownDuration)
{
	const float SafeRemainingSeconds = FMath::Max(0.0f, RemaningSeconds);

	const bool bIsReady = SafeRemainingSeconds <= KINDA_SMALL_NUMBER;

	if (Text_Cooldown)
	{
		if (bIsReady)
		{
			Text_Cooldown->SetText(FText::FromString(TEXT("READY")));
		}
		else
		{
			Text_Cooldown->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), SafeRemainingSeconds)));
		}
	}

	if (Border_Frame)
	{
		const FLinearColor FramColor = bIsReady ? FLinearColor(1.0f, 0.72f, 0.05f, 1.0f) : FLinearColor(0.25f, 0.25f, 0.25f, 1.0);

		Border_Frame->SetBrushColor(FramColor);
	}
}
