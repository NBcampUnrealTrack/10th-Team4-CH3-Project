// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UIManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/HUDWidget.h"
#include "UI/Controller/HUDController.h"

void AUIManager::BeginPlay()
{
	Super::BeginPlay();
	CreateHUDWidget();
}

void AUIManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveHUDWidget();
	Super::EndPlay(EndPlayReason);
}

void AUIManager::TestLowHealthEffect(float CurrentHealth, float MaxHealth)
{
	if (!HUDControllerInstance)
	{
		UE_LOG(LogTemp,
			Warning,
			TEXT("HUDController is not ready")
		);

		return;
	}

	HUDControllerInstance->HandleHealthChanged(CurrentHealth, MaxHealth);

}

void AUIManager::CreateHUDWidget()
{
	if (!HUDWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("HUDWidgetClass is not assigned")
		);
		return;
	}

	if (IsValid(HUDWidgetInstance))
	{
		return;
	}

	if (!IsValid(PlayerOwner))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UIManager has no PlayerOwner")
		);
		return;
	}

	HUDWidgetInstance =
		CreateWidget<UHUDWidget>(PlayerOwner.Get(), HUDWidgetClass);

	if (!IsValid(HUDWidgetInstance))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Failed to create HUDWidget")
		);
		return;
	}

	HUDWidgetInstance->AddToViewport();
	HUDWidgetInstance->SetLowHealthEffectVisible(bPreviewLowHealthEffect);

	HUDControllerInstance = NewObject<UHUDController>(this);

	if (HUDControllerInstance)
	{
		HUDControllerInstance->Initialize(HUDWidgetInstance);
	}
}

void AUIManager::RemoveHUDWidget()
{
	if (HUDControllerInstance)
	{
		HUDControllerInstance->Shutdown();
		HUDControllerInstance = nullptr;
	}

	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}
}


