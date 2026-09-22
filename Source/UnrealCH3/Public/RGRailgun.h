// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "RGBaseWeapon.h"
#include "RGRailgun.generated.h"

//현재 레일건은 라인트레이스 방식으로 공격 . 범위 피해는 아직 구현되지 않음
UCLASS()
class UNREALCH3_API ARGRailgun : public ARGBaseWeapon
{
	GENERATED_BODY()

public:
	// =========== 캐릭터는 레일건에서 좌클릭을 뗄 떼 StopFire()가 아닌 ReleaseChargeAndFire()를 불러와야한다,
	UFUNCTION(BlueprintCallable, Category = "Weapon|Railgun")
	void ReleaseChargeAndFire();

	// UI에서 표시용 . 지금 충전 중인지 반환
	UFUNCTION(BlueprintPure, Category = "Weapon|Railgun")
	bool IsCharging() const { return bIsCharging; }

	// UI 충전 게이지용. 0~1 사이 값 float 반환
	UFUNCTION(BlueprintPure, Category = "Weapon|Railgun")
	float GetChargeRatio01() const;

	//레일건 최소 발사 가능 지점 비율 반환
	UFUNCTION(BlueprintPure, Category = "Weapon|Railgun")
	float GetChargeFireRatio() const;

public:
	// 여기서는 발사 안 하고 충전만 취소함 외부에서 신경 X
	virtual void StopFire() override;

protected:
	// 발사 대신 충전 시작
	virtual void StartFire() override;


	// 실제 관통 트레이스를 반복해서 쏘는 함수
	void FireChargedShot(float ChargeRatio01);

	//레일건 스탯
	UPROPERTY(EditDefaultsOnly, Category = "Railgun")
	float MinChargeTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Railgun")
	float MaxChargeTime = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Railgun")
	float MinDamage = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Railgun")
	float MaxDamage = 140.f;

	// 한 발이 최대 몇 명까지 관통하는지
	UPROPERTY(EditDefaultsOnly, Category = "Railgun")
	int32 MaxPierceCount = 2;

	// 지금 충전 중인지
	bool bIsCharging = false;

	// 충전을 시작한 시각 (초). 지금 시각에서 이걸 빼면 "얼마나 충전했는지"가 나옴.
	float ChargeStartTime = 0.f;

	// 다음 발사가 가능한 시각 , 한번 발사하고 유예시간
	float NextChargeAllowedTime = 0.f;

	// ChargeSpeedUp 강화 배율 (1.0 = 기본, 스택당 EffectAmount만큼 증가)
	float GetChargeSpeedMultiplier() const;

	// =============== 핵심 강화 관련 추가 함수 ================

protected:
	// 핵심 강화 1 빔발사
	bool bIsBeamFiring = false;
	FTimerHandle BeamFireTimerHandle;

	// 끊어 쏴도 정확히 누적되도록, StartBeamFire에서 리셋하지 않고 계속 유지
	float BeamFireElapsedSinceLastAmmoConsumed = 0.f;

	UFUNCTION()
	void HandleBeamFireTick();

	void StartBeamFire();
	void StopBeamFire();

public:
	bool IsBeamFiring() const { return bIsBeamFiring; }

	//핵심 강화 2 호밍 데미지

protected:
	void TryTriggerHomingDamage(const FHitResult& Hit, float DealtDamage);

	//핵심 강화 3 레일건 굵기
protected:
	float GetFireTraceRadius() const;

};