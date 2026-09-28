// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGQuickSlotWidget.generated.h"

class UTextBlock;
class UBorder;
/**
 * 
 */
UCLASS()
class UNREALCH3_API URGQuickSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void ApplyCooldown(float RemaningSeconds, float CooldownDuration);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Cooldown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_Frame;
};
