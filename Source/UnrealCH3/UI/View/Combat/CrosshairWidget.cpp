// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Combat/CrosshairWidget.h"

void UCrosshairWidget::ApplyReloadState(bool bIsReloading, float Progress)
{
	//재장전 시 0~1 범위의 진행률 사용
	const float DisplayProgress = bIsReloading ? FMath::Clamp(Progress, 0.f, 1.f) : 0.f;

	OnReloadVisualUpdate(bIsReloading, DisplayProgress);
}

void UCrosshairWidget::ApplyShotFired()
{
	OnShotVisualRequested();
}

void UCrosshairWidget::ApplyHitConfirmed(bool bKilled)
{
	OnHitVisualRequested(bKilled);
}

