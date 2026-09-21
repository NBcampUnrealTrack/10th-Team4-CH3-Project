// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Combat/RailgunChargeWidget.h"

void URailgunChargeWidget::ApplyChargeState(bool bIsCharging, float ChargeRatio, float MinimumFireRatio, bool bHasAmmo)
{
	const float SafeChargeRatio = FMath::IsFinite(ChargeRatio) ? FMath::Clamp(ChargeRatio, 0.0f, 1.0f) : 0.0f;

	const float SafeMinimumFireRatio = FMath::IsFinite(MinimumFireRatio) ? FMath::Clamp(MinimumFireRatio, 0.0f, 1.0f) : 0.0f;

	const float DisplayChargeRatio = bIsCharging ? SafeChargeRatio : 0.0f;

	const bool bIsReadyToFire = bIsCharging && bHasAmmo && DisplayChargeRatio + KINDA_SMALL_NUMBER >= SafeMinimumFireRatio;

	const bool bIsFullyCharged = bIsCharging && bHasAmmo && DisplayChargeRatio >= 1.0f - KINDA_SMALL_NUMBER;

	OnChargeVisualUpdate(bIsCharging, DisplayChargeRatio, SafeMinimumFireRatio, bIsReadyToFire, bIsFullyCharged, bHasAmmo);
}

void URailgunChargeWidget::ResetChargeState()
{
	OnChargeVisualReset();
}




