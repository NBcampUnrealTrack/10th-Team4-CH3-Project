// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Menu/RGResultWidget.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"

void URGResultWidget::ShowGameOver()
{
	Canvas_GameOverPage->SetVisibility(ESlateVisibility::Visible);
	Canvas_ClearPage->SetVisibility(ESlateVisibility::Collapsed);
}

void URGResultWidget::ShowClear()
{
	Canvas_GameOverPage->SetVisibility(ESlateVisibility::Collapsed);
	Canvas_ClearPage->SetVisibility(ESlateVisibility::Visible);
}

void URGResultWidget::SetGameOverStats(const FText& InSuvivalTime, int32 InEliminations)
{
	Text_SurvivalTimeValue->SetText(InSuvivalTime);
	Text_GameOverEliminationsValue->SetText(FText::AsNumber(InEliminations));
}

void URGResultWidget::SetClearStats(const FText& InClearTime, int32 InEliminations)
{
	Text_ClearTimeValue->SetText(InClearTime);
	Text_ClearEliminationsValue->SetText(FText::AsNumber(InEliminations));
}

void URGResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Retry->OnClicked.AddDynamic(this, &URGResultWidget::HandleRetryClicked);
	Button_StartOver->OnClicked.AddDynamic(this, &URGResultWidget::HandleStartOverClicked);
	Button_MainMenu->OnClicked.AddDynamic(this, &URGResultWidget::HandleMainMenuClicked);
	Button_Exit->OnClicked.AddDynamic(this, &URGResultWidget::HandleExitClicked);

	Button_ClearMainMenu->OnClicked.AddDynamic(this, &URGResultWidget::HandleMainMenuClicked);
	Button_ClearExit->OnClicked.AddDynamic(this, &URGResultWidget::HandleExitClicked);

	ShowGameOver();
}

void URGResultWidget::HandleRetryClicked()
{
	OnRetryRequested.Broadcast();
}

void URGResultWidget::HandleStartOverClicked()
{
	OnStartOverRequested.Broadcast();
}

void URGResultWidget::HandleMainMenuClicked()
{
	OnMainMenuRequested.Broadcast();
}

void URGResultWidget::HandleExitClicked()
{
	OnExitRequested.Broadcast();
}


