// Fill out your copyright notice in the Description page of Project Settings.


#include "RGRailgun.h"
#include "gamemode/RGProgressionSubsystem.h"
#include "Engine/World.h"

//충전 시작만 하는 함수
void ARGRailgun::StartFire() {
	//차징중이거나 CanFire로직에 걸리면 발사 안됨
	if (bIsCharging) {
		return;
	}

	if (!CanFire()) {
		return;
	}
	// 발사 간격 시간 이후에만 발사 가능
	if (GetWorld()->GetTimeSeconds() < NextChargeAllowedTime) {
		return;
	}
	//충전중 ture
	bIsCharging = true;
	//차징 시작 시간을 계산하는게 주 임무
	ChargeStartTime = GetWorld()->GetTimeSeconds();
}
//충전 위한 변수 초기화 함수
void ARGRailgun::StopFire() {
	//충전 중 아님으로 설정
	bIsCharging = false;
	//차징 시작 시간 0초로 초기화
	ChargeStartTime = 0.f;
	Super::StopFire();
}
//충전이 얼마나 되었는지 0~1 사이 값을 반환
float ARGRailgun::GetChargeRatio01() const {
	if (!bIsCharging) {
		return 0.f;
	}
	// 실제 흐른 시간에 강화 배율을 곱해서 "체감 충전 시간"을 늘림
	const float RawElapsed = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	//차징 시간 ScaledElapsed에 저장
	const float ScaledElapsed = RawElapsed * GetChargeSpeedMultiplier();
	//차징 시간 0~MaxChargeTime으로 한정시킴
	const float ClampedElapsed = FMath::Clamp(ScaledElapsed, 0.f, MaxChargeTime);
	// 0~1비율로 반환
	return ClampedElapsed / MaxChargeTime;
}

void ARGRailgun::ReleaseChargeAndFire() {
	if (!bIsCharging) {
		return;
	}
	//차징시간
	
	const float RawElapsed = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	const float Elapsed = RawElapsed * GetChargeSpeedMultiplier();
	//차징이 끝났으므로
	bIsCharging = false;
	//최소 충전 시간 미만족시 그냥 리턴
	if (Elapsed < MinChargeTime) {
		return;
	}
	//충전 시간 0~1 비율로 변환
	const float ClampedElapsed = FMath::Clamp(Elapsed, MinChargeTime, MaxChargeTime);
	const float ChargeRatio = (ClampedElapsed - MinChargeTime) / (MaxChargeTime - MinChargeTime);

	FireChargedShot(ChargeRatio);

	//탄환 한발 소모 및 브로드캐스팅
	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	OnAmmoChanged.Broadcast(CurrentAmmo, GetMagazineCapacity());

	if (CurrentAmmo <= 0) {
		StartReloaded();
	}

	// 다음 충전은 발사 간격 이후에 가능.
	NextChargeAllowedTime = GetWorld()->GetTimeSeconds() + WeaponStats.FireInterval;
}
//실제로 총을 쏘는 부분
void ARGRailgun::FireChargedShot(float ChargeRatio01) {
	
	//탄퍼짐을 고려한 실제 발사 스프레드를 FireDirection에 넣음
	FVector StartLocation, FireDirection;
	if (!GetMuzzleAimTransform(StartLocation, FireDirection))
	{
		return;
	}
	FireDirection = ApplySpread(FireDirection);

	// 충전을 조금 했으면 MinDamage에 가깝게, 완전 충전했으면 MaxDamage에 가깝게
	const float Damage = FMath::Lerp(MinDamage, MaxDamage, ChargeRatio01);

	//같은 방향으로 여러번 쏘면서 이미 맞은 적은 무시 대상에 넣으면 관통 처리 완료.
	TSet<AActor*> AlreadyHitActors;
	for (int32 i = 0; i < MaxPierceCount; i++) {
		const bool bHit = FireHitscan(StartLocation, FireDirection, Damage, &AlreadyHitActors);
		if (!bHit) {
			break;
		}
	}
}

float ARGRailgun::GetChargeSpeedMultiplier() const
{
	float Multiplier = 1.0f;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks = Progression->GetUpgradeStackCount(FName(TEXT("RateUp")));
			const float EffectAmount = Progression->GetUpgradeEffectAmount(FName(TEXT("RateUp")));
			Multiplier += EffectAmount * Stacks; // 덧셈형이라 음수/0 될 일 없음
		}
	}
	return Multiplier;
}

