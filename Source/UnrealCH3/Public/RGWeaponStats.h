// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RGWeaponStats.generated.h"

/**
 * 무기 1종의 기본 스탯 행. Content Browser에서 이 구조체로 DataTable 애셋(DT_WeaponBaseStats)을 만들고,
 * RowName을 "AssaultRifle", "Shotgun", "Railgun" 등으로 채워서 사용합니다.
 */
USTRUCT(BlueprintType)
struct FWeaponStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	// 기본 피해량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float BaseDamage = 12.f;

	// 발사 간격 (기본 0.1초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float FireInterval = 0.1f;

	// 탄창 용량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MagazineCapacity = 30;

	// 전체 재장전 소요 시간(초). 한 발씩 장전하는 무기는 "한 발당" 시간으로 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float ReloadTime = 1.8f;

	// 조준시 카메라 FOV 배율 <- 모든 무기 고정으로 해도 문제 X
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float ADSFOVMultiplier = 1.35f;

	// 거리 감쇠 시작/끝 거리. 둘 다 0 이하면 "감쇠 없음"(레일건)으로 처리
	// 거리 감쇠에 대한 부분은 밸런스에서 처리 or 회의 필요할 듯
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DamageFalloffStart = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DamageFalloffEnd = 5000.f;

	// 최대 감쇠 거리에서의 최종 피해 배율(하한선)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MinFalloffDamageMultiplier = 0.5f;

	// true = 재장전 시 탄창 전체 교체(돌격소총/레일건), false = 한 발씩 장전(산탄총)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bReloadWholeMagazine = true;
};