// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrosshairWidget.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API UCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//Controller 가 전달한 재장전 상태를 UI 에 반영
	UFUNCTION(BlueprintCallable, Category = "UI|Reload")
	void ApplyReloadState(bool bIsReloading, float Progress);
	// 발사 알림 전달
	void ApplyShotFired();
	
	// 피해 확인
	void ApplyHitConfirmed(bool bKilled);
	
protected:
	// 재장전 UI 비주얼 업데이트 함수 BP 에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Reload")
	void OnReloadVisualUpdate(bool bIsReloading, float Progress);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Crosshair")
	void OnShotVisualRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Crosshair")
	void OnHitVisualRequested(bool bKilled);
};
