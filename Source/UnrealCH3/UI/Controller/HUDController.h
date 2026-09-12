// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HUDController.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnHUDDamageNumberRequested, float, AActor*, FVector);

class UHUDWidget;
class ARGBaseWeapon;
class UWeaponInfoWidget;
class UCrosshairWidget;
class UWorld;

/**
 * 
 */
UCLASS()
class UNREALCH3_API UHUDController : public UObject
{
	GENERATED_BODY()
	
public:
	//Controller HUD 연결
	void Initialize(UHUDWidget* InHUDWidget);

	//Model 체력 연결 함수
	void HandleHealthChanged(float CurrentHealth, float MaxHelth);

	//HUD 연결 해제 함수
	void Shutdown();

	// 무기 정보 표시 view 등록
	void SetWeaponInfoView(UWeaponInfoWidget* InWeaponInfoView);

	// 무기 연결 및 초기 탄약 표시
	void BindWeapon(ARGBaseWeapon* InWeapon, const FText& InWeaponDisplayName);

	// 무기 연결 해제
	void UnbindWeapon();

	//크로스헤어 등록
	void SetCrosshairView(UCrosshairWidget* InCrosshairView);

	FOnHUDDamageNumberRequested OnDamageNumberRequested;

protected:
	//HUDWidget 약한 참조 연결
	TWeakObjectPtr<UHUDWidget> HUDWidget;

	//LowHealthRatio 이펙트 나오는 값 조정 (50% 시작 최대 25%)
	float LowHealthFadeStartRatio = 0.5f;
	float LowHealthFullIntensityRatio = 0.25f;

	// 무기와 view 약한 참조
	TWeakObjectPtr<ARGBaseWeapon> BoundWeapon;
	TWeakObjectPtr<UWeaponInfoWidget> WeaponInfoView;

	FText BoundWeaponDisplayName;

	UFUNCTION()
	void HandleWeaponAmmoChanged(int32 CurrentAmmo, int32 MagazineCapacity);

	UFUNCTION()
	void HandleWeaponReloadStarted();
	
	UFUNCTION()
	void HandleWeaponReloadCompleted();
	
	UFUNCTION()
	void HandleWeaponReloadCanceled();

	void RefreshWeaponInfo();

	TWeakObjectPtr<UCrosshairWidget> CrosshairView;

	// 재장전 UI 갱신 타이머
	FTimerHandle ReloadProgressTimerHandle;

	//타이머 연결 World 해제
	TWeakObjectPtr<UWorld> ReloadTimerWorld;

	// 현재 무기의 재장전 시간과 타이머 동기화
	void RefreshReloadUI();

	// 무기의 재장전 진행률 크로스헤어에 전달
	void UpdateReloadProgress();

	// UI 타이머 해제
	void StopReloadProgressTimer();

	UFUNCTION()
	void HandleWeaponShotFired();

	UFUNCTION()
	void HandleWeaponDamageConfirmed(float AppliedDamage, bool bKilled);

	UFUNCTION()
	void HandleWeaponDamageNumberRequested(float AppliedDamage, AActor* TargetActor, FVector WorldLocation);

};
