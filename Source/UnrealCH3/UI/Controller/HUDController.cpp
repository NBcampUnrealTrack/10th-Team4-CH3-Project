// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Controller/HUDController.h"
#include "UI/HUDWidget.h"

//Controller가 제어할 HUD를 저장 후 초기값 셋팅
void UHUDController::Initialize(UHUDWidget* InHUDWidget)
{
	HUDWidget = InHUDWidget;

	if (HUDWidget.IsValid())
	{
		HUDWidget->SetLowHealthEffectIntensity(0.0f);
	}
}

void UHUDController::HandleHealthChanged(float CurrentHealth, float MaxHelth)
{
	if (!HUDWidget.IsValid())
	{
		return;
	}

	if (MaxHelth <= 0.0f)
	{
		HUDWidget->SetLowHealthEffectIntensity(0.0f);
		return;
	}

	// 현재 체력 / 최대체력으로 나누어 HealthRatio 계산
	const float HealthRatio =
		FMath::Clamp(
			CurrentHealth / MaxHelth,
			0.0f,
			1.0f
		);


	// 체력 비율에 따라 효과 강도 저장
	const float Intensity =
		FMath::GetMappedRangeValueClamped(
			FVector2D(
				LowHealthFullIntensityRatio,
				LowHealthFadeStartRatio
			),
			FVector2D(1.0f, 0.0f),
			HealthRatio
		);

	//Intensity 값에 따라 LowHealthEffect 의 강도 설정
	HUDWidget->SetLowHealthEffectIntensity(Intensity);
}

void UHUDController::Shutdown()
{
	if (HUDWidget.IsValid())
	{
		HUDWidget->SetLowHealthEffectIntensity(0.0f);
	}

	HUDWidget.Reset();

}




