// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UITypes.h"
#include "HUDWidget.generated.h"

class UImage;
class UOverlay;
class URGQuickSlotWidget;

/**
 * 
 */
UCLASS()
class UNREALCH3_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// LowHealthEffect 끄기/켜기
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void SetLowHealthEffectVisible(bool bVisible);

	// LowHealthEffect 강도 적용
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void SetLowHealthEffectIntensity(float Intensity);

	UOverlay* GetLayer(EUILayer Layer) const;

	URGQuickSlotWidget* GetHealQuickSlotView() const { return QuickSlot_Heal; }

	URGQuickSlotWidget* GetGrenadeQuickSlotView() const { return QuickSlot_Grenade; }


protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_LowHealthVignette;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_GameLayer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_WorldLayer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_NotificationLayer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_SelectionLayer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_MenuLayer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_TransitionLayer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_ResultLayer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URGQuickSlotWidget> QuickSlot_Heal;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URGQuickSlotWidget> QuickSlot_Grenade;
	
};
