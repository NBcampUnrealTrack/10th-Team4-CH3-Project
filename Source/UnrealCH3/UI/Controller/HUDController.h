// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HUDController.generated.h"

class UHUDWidget;
class ARGBaseWeapon;
class UWeaponInfoWidget;

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

	void RefreshWeaponInfo();

};
