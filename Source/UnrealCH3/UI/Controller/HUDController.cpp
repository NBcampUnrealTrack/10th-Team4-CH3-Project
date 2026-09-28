// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Controller/HUDController.h"
#include "UI/HUDWidget.h"
#include "Public/RGBaseWeapon.h"
#include "UI/View/Combat/WeaponInfoWidget.h"
#include "UI/View/Combat/CrosshairWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Public/RGRailgun.h"
#include "UI/View/Combat/RailgunChargeWidget.h"
#include "UI/View/Combat/RGQuickSlotWidget.h"
#include "Player/RGCharacter.h"
#include "RGGrenade.h"
#include "UI/View/Combat/RGQuickSlotWidget.h"

//Controller가 제어할 HUD를 저장 후 초기값 셋팅
void UHUDController::Initialize(UHUDWidget* InHUDWidget)
{
	HUDWidget = InHUDWidget;

	if (HUDWidget.IsValid())
	{
		HUDWidget->SetLowHealthEffectIntensity(0.0f);
	}
}

void UHUDController::HandleHealthChanged(float CurrentHealth, float MaxHelth)
{
	if (!HUDWidget.IsValid())
	{
		return;
	}

	if (MaxHelth <= 0.0f)
	{
		HUDWidget->SetLowHealthEffectIntensity(0.0f);
		return;
	}

	// 현재 체력 / 최대체력으로 나누어 HealthRatio 계산
	const float HealthRatio =
		FMath::Clamp(
			CurrentHealth / MaxHelth,
			0.0f,
			1.0f
		);


	// 체력 비율에 따라 효과 강도 저장
	const float Intensity =
		FMath::GetMappedRangeValueClamped(
			FVector2D(
				LowHealthFullIntensityRatio,
				LowHealthFadeStartRatio
			),
			FVector2D(1.0f, 0.0f),
			HealthRatio
		);

	//Intensity 값에 따라 LowHealthEffect 의 강도 설정
	HUDWidget->SetLowHealthEffectIntensity(Intensity);
}

void UHUDController::Shutdown()
{
	UnbindWeapon();
	WeaponInfoView.Reset();

	if (HUDWidget.IsValid())
	{
		HUDWidget->SetLowHealthEffectIntensity(0.0f);
	}

	HUDWidget.Reset();

	CrosshairView.Reset();

	if (URailgunChargeWidget* View = RailgunChargeView.Get())
	{
		View->ResetChargeState();
	}

	RailgunChargeView.Reset();

}

void UHUDController::SetWeaponInfoView(UWeaponInfoWidget* InWeaponInfoView)
{
	WeaponInfoView = InWeaponInfoView;

	RefreshWeaponInfo();
}

void UHUDController::BindWeapon(ARGBaseWeapon* InWeapon, const FText& InWeaponDisplayName)
{
	UnbindWeapon();

	if (!IsValid(InWeapon))
	{
		return;
	}

	BoundWeapon = InWeapon;
	BoundWeaponDisplayName = InWeaponDisplayName;

	InWeapon->OnAmmoChanged.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponAmmoChanged
	);
	
	InWeapon->OnReloadStarted.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponReloadStarted
	);
	
	InWeapon->OnReloadCompleted.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponReloadCompleted
	);
	
	InWeapon->OnReloadCanceled.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponReloadCanceled
	);

	InWeapon->OnShotFired.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponShotFired
	);

	InWeapon->OnDamageConfirmed.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponDamageConfirmed
	);

	InWeapon->OnDamageNumberRequested.AddUniqueDynamic(
		this,
		&UHUDController::HandleWeaponDamageNumberRequested
	);

	RefreshWeaponInfo();
	RefreshRailgunChargeUI();
}

void UHUDController::UnbindWeapon()
{
	StopReloadProgressTimer();
	StopRailgunChargeTimer();

	if (UCrosshairWidget* View = CrosshairView.Get())
	{
		View->ApplyReloadState(false, 0.0f);
	}

	if (URailgunChargeWidget* View = RailgunChargeView.Get())
	{
		View->ResetChargeState();
	}

	if (ARGBaseWeapon* Weapon = BoundWeapon.Get())
	{
		Weapon->OnAmmoChanged.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponAmmoChanged
		);

		Weapon->OnReloadStarted.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponReloadStarted
		);

		Weapon->OnReloadCompleted.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponReloadCompleted
		);

		Weapon->OnReloadCanceled.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponReloadCanceled
		);

		Weapon->OnShotFired.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponShotFired
		);

		Weapon->OnDamageConfirmed.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponDamageConfirmed
		);

		Weapon->OnDamageNumberRequested.RemoveDynamic(
			this,
			&UHUDController::HandleWeaponDamageNumberRequested
		);

	}

	BoundWeapon.Reset();
	BoundWeaponDisplayName = FText::GetEmpty();
}

void UHUDController::SetCrosshairView(UCrosshairWidget* InCrosshairView)
{
	if (UCrosshairWidget* PreviousView = CrosshairView.Get())
	{
		PreviousView->ApplyReloadState(false, 0.f);
	}

	CrosshairView = InCrosshairView;
	RefreshReloadUI();
}

void UHUDController::SetRailgunChargeView(URailgunChargeWidget* InRailgunChargeView)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RailgunUI] SetRailgunChargeView called: %s"),
		IsValid(InRailgunChargeView)
		? *InRailgunChargeView->GetName()
		: TEXT("NULL")
	);

	if (URailgunChargeWidget* PreviousView = RailgunChargeView.Get())
	{
		PreviousView->ResetChargeState();
	}

	RailgunChargeView = InRailgunChargeView;

	if (URailgunChargeWidget* NewView = RailgunChargeView.Get())
	{
		NewView->ResetChargeState();
	}

	RefreshRailgunChargeUI();
}

void UHUDController::SetQuickSlotViews(URGQuickSlotWidget* InHealQuikSlotView, URGQuickSlotWidget* InGrenadeQuickSlotView)
{
	HealQuickSlotView = InHealQuikSlotView;
	GrenadeQuickSlotView = InGrenadeQuickSlotView;

	RefreshQuickSlotUI();
}

void UHUDController::BindQuickSlotCharacter(ARGCharacter* InCharacter)
{
	QuickSlotCharacter = InCharacter;

	RefreshQuickSlotUI();
}

void UHUDController::HandleWeaponAmmoChanged(int32 CurrentAmmo, int32 MagazineCapacity)
{
	if (!BoundWeapon.IsValid())
	{
		return;
	}

	if (UWeaponInfoWidget* View = WeaponInfoView.Get())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponUI][AmmoChanged] %s / %d"),
			*BoundWeaponDisplayName.ToString(),
			CurrentAmmo);

		View->ApplyWeaponInfo(
			BoundWeaponDisplayName,
			CurrentAmmo
		);
	}
}

void UHUDController::RefreshWeaponInfo()
{
	ARGBaseWeapon* Weapon = BoundWeapon.Get();
	UWeaponInfoWidget* View = WeaponInfoView.Get();

	if (!IsValid(Weapon) || !IsValid(View))
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[WeaponUI][Refresh] %s / %d"),
		*BoundWeaponDisplayName.ToString(),
		Weapon->GetCurrentAmmo());

	View->ApplyWeaponInfo(
		BoundWeaponDisplayName,
		Weapon->GetCurrentAmmo()
	);
}

void UHUDController::RefreshReloadUI()
{
	//이전 갱신 초기화
	StopReloadProgressTimer();
	UpdateReloadProgress();

	ARGBaseWeapon* Weapon = BoundWeapon.Get();

	if (!CrosshairView.IsValid() || !Weapon || !Weapon->IsReloading())
	{
		return;
	}

	UWorld* World = Weapon->GetWorld();

	if (!World)
	{
		return;
	}

	ReloadTimerWorld = World;

	// 재장전 중 60프레임으로 화면 갱신
	World->GetTimerManager().SetTimer(
		ReloadProgressTimerHandle,
		this,
		&UHUDController::UpdateReloadProgress,
		1.f / 60.f,
		true
	);
}

void UHUDController::UpdateReloadProgress()
{
	UCrosshairWidget* View = CrosshairView.Get();
	ARGBaseWeapon* Weapon = BoundWeapon.Get();

	if (!View)
	{
		StopReloadProgressTimer();
		return;
	}

	if (!Weapon || !Weapon->IsReloading())
	{
		StopReloadProgressTimer();
		View->ApplyReloadState(false, 0.f);
		return;
	}

	View->ApplyReloadState(true, Weapon->GetReloadProgress());
}

void UHUDController::StopReloadProgressTimer()
{
	if (UWorld* World = ReloadTimerWorld.Get())
	{
		World->GetTimerManager().ClearTimer(ReloadProgressTimerHandle);
	}

	ReloadProgressTimerHandle.Invalidate();
	ReloadTimerWorld.Reset();
}

void UHUDController::HandleWeaponShotFired()
{
	if (UCrosshairWidget* View = CrosshairView.Get())
	{
		View->ApplyShotFired();
	}
}

void UHUDController::HandleWeaponDamageConfirmed(float AppliedDamage, bool bKilled)
{
	if (AppliedDamage <= 0.f)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[HUDController] Confirmed damage: %.1f / Killed: %s"),
		AppliedDamage,
		bKilled ? TEXT("true") : TEXT("false")
	);

	if (UCrosshairWidget* View = CrosshairView.Get())
	{
		View->ApplyHitConfirmed(bKilled);
	}
}

void UHUDController::HandleWeaponDamageNumberRequested(float AppliedDamage, AActor* TargetActor, FVector WorldLocation)
{
	if (AppliedDamage <= 0.f)
	{
		return;
	}

	OnDamageNumberRequested.Broadcast(AppliedDamage, TargetActor, WorldLocation);
}

void UHUDController::RefreshRailgunChargeUI()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RailgunUI] Refresh called / View=%s / Weapon=%s"),
		RailgunChargeView.IsValid() ? TEXT("Valid") : TEXT("Invalid"),
		BoundWeapon.IsValid() ? *BoundWeapon->GetName() : TEXT("NULL")
	);

	StopRailgunChargeTimer();

	URailgunChargeWidget* View = RailgunChargeView.Get();

	if (!IsValid(View))
	{
		return;
	}

	View->ResetChargeState();

	ARGRailgun* Railgun = Cast<ARGRailgun>(BoundWeapon.Get());

	if (!IsValid(Railgun))
	{
		return;
	}

	UWorld* World = Railgun->GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	RailgunChargeTimerWorld = World;

	UpdateRailgunChargeProgress();

	World->GetTimerManager().SetTimer(
		RailgunChargeTimerHandle,
		this,
		&UHUDController::UpdateRailgunChargeProgress,
		1.0f / 60.0f,
		true
	);
}

void UHUDController::UpdateRailgunChargeProgress()
{
	URailgunChargeWidget* View = RailgunChargeView.Get();

	if (!IsValid(View))
	{
		StopRailgunChargeTimer();
		return;
	}

	ARGRailgun* Railgun = Cast<ARGRailgun>(BoundWeapon.Get());

	if (!IsValid(Railgun))
	{
		View->ResetChargeState();
		StopRailgunChargeTimer();
		return;
	}

	const bool bIsCharging = Railgun->IsCharging();
	const float ChargeRatio = Railgun->GetChargeRatio01();
	const float MinimumFireRatio = Railgun->GetChargeFireRatio();
	const bool bHasAmmo = Railgun->GetCurrentAmmo() > 0;

	View->ApplyChargeState(
		bIsCharging,
		ChargeRatio,
		MinimumFireRatio,
		bHasAmmo
	);
}

void UHUDController::StopRailgunChargeTimer()
{
	if (UWorld* World =
		RailgunChargeTimerWorld.Get())
	{
		World->GetTimerManager().ClearTimer(
			RailgunChargeTimerHandle
		);
	}

	RailgunChargeTimerHandle.Invalidate();
	RailgunChargeTimerWorld.Reset();
}

void UHUDController::RefreshQuickSlotUI()
{
	StopQuickSlotTimer();

	ARGCharacter* Character = QuickSlotCharacter.Get();

	if (!IsValid(Character))
	{
		return;
	}

	if (!HealQuickSlotView.IsValid()&&!GrenadeQuickSlotView.IsValid())
	{
		return;
	}

	UWorld* World = Character->GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	QickSlotTimerWorld = World;

	UpdateQuickSlotUI();

	World->GetTimerManager().SetTimer(
		QuickSlotTimerHandle,
		this,
		&UHUDController::UpdateQuickSlotUI,
		0.05f,
		true
	);
}

void UHUDController::UpdateQuickSlotUI()
{
	ARGCharacter* Character = QuickSlotCharacter.Get();

	if (!IsValid(Character))
	{
		StopQuickSlotTimer();
		return;
	}

	if (URGQuickSlotWidget* HealView = HealQuickSlotView.Get())
	{
		HealView->ApplyCooldown(Character->GetHealCooldownRemaining(), 0.0f);
	}

	if (URGQuickSlotWidget* GrenadeView = GrenadeQuickSlotView.Get())
	{
		GrenadeView->ApplyCooldown(ARGGrenade::GetGrenadeCooldownRemaining(Character), 0.0f);
	}
}

void UHUDController::StopQuickSlotTimer()
{
	if (UWorld* World = QickSlotTimerWorld.Get())
	{
		World->GetTimerManager().ClearTimer(QuickSlotTimerHandle);
	}

	QuickSlotTimerHandle.Invalidate();
	QickSlotTimerWorld.Reset();
}

void UHUDController::HandleWeaponReloadStarted()
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[HUDController] Reload started")
	);

	RefreshReloadUI();
}

void UHUDController::HandleWeaponReloadCompleted()
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[HUDController] Reload completed")
	);

	RefreshReloadUI();
}

void UHUDController::HandleWeaponReloadCanceled()
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[HUDController] Reload canceled")
	);

	RefreshReloadUI();
}


