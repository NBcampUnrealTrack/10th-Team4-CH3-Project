// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RailgunChargeWidget.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API URailgunChargeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//레일건 충전 상태
	UFUNCTION(BlueprintCallable, Category = "UI|Railgun")
	void ApplyChargeState(bool bIsCharging, float ChargeRatio, float MinimumFireRatio, bool bHasAmmo);
	
	//레일건 ui 초기화
	UFUNCTION(BlueprintCallable, Category = "UI|Railgun")
	void ResetChargeState();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Railgun")
	void OnChargeVisualUpdate(bool bIsCharging, float ChargRatio, float MinimumFireRatio, bool bIsReadyToFire, bool bIsFullCharged, bool bHasAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Railgun")
	void OnChargeVisualReset();

};
