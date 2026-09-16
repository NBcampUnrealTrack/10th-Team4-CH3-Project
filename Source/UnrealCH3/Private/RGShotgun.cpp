// Fill out your copyright notice in the Description page of Project Settings.
#include "RGShotgun.h"

void ARGShotgun::StartFire()
{
	// 산탄총은 한 발씩 장전하는 방식이라, 장전 중 발사 버튼을 누르면
	// 진행 중이던 장전을 끝까지 기다리지 않고 즉시 취소한 뒤 바로 발사 가능한 상태로 전환.
	if (IsReloading())
	{
		CancelReloaded();
	}
	Super::StartFire();
}

void ARGShotgun::Fire()
{
	// 카메라 위치/방향은 돌격소총과 동일하게 한 번만 구함 
	FVector StartLocation, FireDirection;
	if (!GetMuzzleAimTransform(StartLocation, FireDirection))
	{
		return;
	}

	// 펠릿 하나마다 별도로 퍼짐(ApplySpread)을 적용해서 각기 다른 방향으로 히트스캔 발사.
	// 펠릿 하나당 피해량은 WeaponStats.BaseDamage(데이터테이블 값, 예: 8)를 그대로 사용 ->
	// 여러 발이 맞으면 ApplyHitDamage가 그만큼 여러 번 호출되어 결과적으로 총피해가 합산됨.
	for (int32 i = 0; i < PelletCount; ++i)
	{
		const FVector PelletDirection = ApplySpread(FireDirection);
		FireHitscan(StartLocation, PelletDirection, -1.f, nullptr);
	}
}