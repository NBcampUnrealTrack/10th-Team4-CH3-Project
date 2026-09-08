// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/UITypes.h"
#include "UIManager.generated.h"

class UHUDWidget;
class UHUDController;
class UUserWidget;

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

	//메뉴 입력 모드
	UPROPERTY(Transient)
	bool bIsMenuInputModeActive = false;

	//테스트용 변수 모음
	UPROPERTY(EditDefaultsOnly, Category = "UI|Debug")
	bool bPreviewLowHealthEffect = false;

	UPROPERTY(EditDefaultsOnly, Category = "UI|View")
	TSubclassOf<UUserWidget> PauseMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Debug")
	TSubclassOf<UUserWidget> TestSelectionClass;

	UFUNCTION()
	void HandleResumeRequested();

	//View 저장 Map 생성
	UPROPERTY(Transient)
	TMap<TSubclassOf<UUserWidget>, TObjectPtr<UUserWidget>> ActiveViews;

	//View Layer 저장 Map 생성
	UPROPERTY(Transient)
	TMap<TSubclassOf<UUserWidget>, EUILayer> ActiveViewLayers;

	//해당 Layer 게임 입력 Blocking 판단
	bool IsInputBlockingLayer(EUILayer Layer) const;
	//입력을 막는 Layer 존재 여부 판단
	bool HasInputBlockingView() const;
	//Layer 에 활성화 된 View 존재 여부 판단
	bool HasActiveViewInLayer(EUILayer Layer) const;

public:
	// 테스트용 함수 모음
	UFUNCTION(Exec, BlueprintCallable, Category = "UI|Debug")
	void TestOpenPauseMenu();

	UFUNCTION(Exec)
	void TestLowHealthEffect(float CurrentHealth, float MaxHealth);
	
	UFUNCTION(Exec)
	void TestOpenSelection();
	
	UFUNCTION(Exec)
	void TestCloseSelection();

	//HUD 생성
	UFUNCTION(Exec)
	void CreateHUDWidget();

	//HUD 제거
	UFUNCTION(Exec)
	void RemoveHUDWidget();

	//HUD 표시 (Move, Look 사용)
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void ApplyGameInputMode();

	//일시정지, 강화, 결과 화면 등 표시 ( 이동 입력 차단, 마우스 표시)
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void ApplyMenuInputMode();

	//View 계층 추가
	UFUNCTION(BlueprintCallable, Category = "UI|View")
	UUserWidget* OpenView(TSubclassOf<UUserWidget> ViewClass, EUILayer Layer);

	//View 계층 제거
	UFUNCTION(BlueprintCallable, Category = "UI|View")
	bool CloseView(TSubclassOf<UUserWidget> ViewClass);
};
