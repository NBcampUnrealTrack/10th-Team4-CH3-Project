// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUDWidget.h"
#include "Components/Image.h"

void UHUDWidget::SetLowHealthEffectVisible(bool bVisible)
{
	if (!Image_LowHealthVignette)
	{
		return;
	}

	if (bVisible)
	{
		Image_LowHealthVignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Image_LowHealthVignette->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHUDWidget::SetLowHealthEffectIntensity(float Intensity)
{
	if (!Image_LowHealthVignette)
	{
		return;
	}

	//Intensity 의 강도 값음 0.0 ~ 1.0 사이로 고정
	const float ClampedIntensity =
		FMath::Clamp(Intensity, 0.0f, 1.0f);

	//ClampedIntensity의 값이 0에 가까울 수록 Collapsed 처리
	if (ClampedIntensity <= KINDA_SMALL_NUMBER)
	{
		Image_LowHealthVignette->SetRenderOpacity(0.0f);
		Image_LowHealthVignette->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	//LowHealthVignette 의 불투명도 설정
	Image_LowHealthVignette->SetRenderOpacity(ClampedIntensity);
	Image_LowHealthVignette->SetVisibility(ESlateVisibility::HitTestInvisible);

}
