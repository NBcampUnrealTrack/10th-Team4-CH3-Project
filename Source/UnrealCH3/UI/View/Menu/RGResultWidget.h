// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGResultWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;

/**
 * 
 */

DECLARE_MULTICAST_DELEGATE(FOnResultAction);

UCLASS()
class UNREALCH3_API URGResultWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FOnResultAction OnRetryRequested;
	FOnResultAction OnStartOverRequested;
	FOnResultAction OnMainMenuRequested;
	FOnResultAction OnExitRequested;

	void ShowGameOver();
	void ShowClear();

	void SetGameOverStats(const FText& InSuvivalTime, int32 InEliminations);
	void SetClearStats(const FText& InClearTime, int32 InEliminations);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Canvas_GameOverPage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Canvas_ClearPage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Retry;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_StartOver;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_MainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Exit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_ClearMainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_ClearExit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SurvivalTimeValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GameOverEliminationsValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ClearTimeValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ClearEliminationsValue;

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleStartOverClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleExitClicked();
};
