// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UImage;

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

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_LowHealthVignette;
	
};
