// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponInfoWidget.generated.h"

class UTextBlock;
class UBorder;

/**
 * 
 */

UCLASS()
class UNREALCH3_API UWeaponInfoWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Weapon")
	void ApplyWeaponInfo(const FText& WeaponName, int32 CurrentAmmo);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_WeaponName;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AmmoCurrent;
	
	UPROPERTY(BlueprintReadOnly, Category = "UI|Weapon", meta = (BindWidget))
	TObjectPtr<UBorder> Border_WeaponPanel;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Weapon", meta = (ClmapMin = "0.01"))
	float InfoVisibleDuration = 2.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Weapon")
	void OnWeaponInfoShowRequested();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Weapon")
	void OnWeaponInfoFadeOutRequested();

private:
	FTimerHandle WeaponInfoHideTimerHandle;

	void RefreshWeaponInfoVisibility();
	void RequestWeaponInfoFadeOut();

};
