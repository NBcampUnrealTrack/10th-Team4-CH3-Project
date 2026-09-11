// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUDWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"

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

UOverlay* UHUDWidget::GetLayer(EUILayer Layer) const
{
	switch (Layer)
	{
	case EUILayer::Game:
		return Overlay_GameLayer;

	case EUILayer::World:
		return Overlay_WorldLayer;

	case EUILayer::Notification:
		return Overlay_NotificationLayer;

	case EUILayer::Selection:
		return Overlay_SelectionLayer;

	case EUILayer::Menu:
		return Overlay_MenuLayer;

	case EUILayer::Transition:
		return Overlay_TransitionLayer;

	case EUILayer::Result:
		return Overlay_ResultLayer;
	}

	return nullptr;
}

