// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UIManager.generated.h"

class UHUDWidget;
class UHUDController;

/**
 * 
 */
UCLASS()
class UNREALCH3_API AUIManager : public AHUD
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	//실제 화면
	UPROPERTY(Transient)
	TObjectPtr<UHUDWidget> HUDWidgetInstance;

	//화면에 값 전달
	UPROPERTY(Transient)
	TObjectPtr<UHUDController> HUDControllerInstance;

	//테스트용 변수 모음
	UPROPERTY(EditDefaultsOnly, Category = "UI|Debug")
	bool bPreviewLowHealthEffect = false;

public:
	UFUNCTION(Exec)
	void TestLowHealthEffect(float CurrentHealth, float MaxHealth);

	//HUD 생성
	void CreateHUDWidget();

	//HUD 제거
	void RemoveHUDWidget();
};
