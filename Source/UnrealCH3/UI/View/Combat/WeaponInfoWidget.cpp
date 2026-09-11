// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Combat/WeaponInfoWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UWeaponInfoWidget::ApplyWeaponInfo(const FText& WeaponName, int32 CurrentAmmo)
{
	if (IsValid(Text_WeaponName))
	{
		Text_WeaponName->SetText(WeaponName);
	}
	
	if (IsValid(Text_AmmoCurrent))
	{
		const int32 DisplayAmmo = FMath::Max(0, CurrentAmmo);

		Text_AmmoCurrent->SetText(FText::AsNumber(DisplayAmmo));
	}

	RefreshWeaponInfoVisibility();
}

void UWeaponInfoWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WeaponInfoHideTimerHandle);
	}

	Super::NativeDestruct();
}

void UWeaponInfoWidget::RefreshWeaponInfoVisibility()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	TimerManager.ClearTimer(WeaponInfoHideTimerHandle);

	TimerManager.SetTimer(
		WeaponInfoHideTimerHandle,
		this,
		&UWeaponInfoWidget::RequestWeaponInfoFadeOut,
		FMath::Max(0.01f, InfoVisibleDuration),
		false
	);

	OnWeaponInfoShowRequested();
}

void UWeaponInfoWidget::RequestWeaponInfoFadeOut()
{
	OnWeaponInfoFadeOutRequested();
}






