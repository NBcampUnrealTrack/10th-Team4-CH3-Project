// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RGBaseWeapon.h"
#include "RGUpgradeOption.generated.h"

/*
 UI(WBP_UpgradeSelection)의 "Set Upgrade Card Data" 노드 핀 이름과 1:1로 맞춘 구조체.
 서브시스템이 후보 N개를 이 구조체 배열로 뽑아서 UI에 통째로 넘겨줌.
 */
USTRUCT(BlueprintType)
struct FRGUpgradeOption
{
	GENERATED_BODY()

	// 카드 클릭 결과를 다시 받을 때 "이게 뭔지" 구분하는 실제 키값 (예: "DamageUp")
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FName UpgradeId;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FText CategoryText;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FText NameText;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FText DescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FText ValueChangeText;
};

/*
 DT_GeneralUpgrade용 행 구조체. RowName = UpgradeId (예: "DamageUp", "FireRateUp").
 */
USTRUCT(BlueprintType)
struct FRGGeneralUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText CategoryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText NameText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText DescriptionText;

	// UI 카드에 표시할 문구. 예: "DAMAGE: 100 -> 120"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText ValueChangeText;

	// 최대 중첩 횟수 (예외사항: "최대 중첩 후보 제외")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 MaxStack = 3;

	// 스택 1당 실제 효과량 (예: DamageUp이면 0.08 = +8%). 무기/캐릭터 스탯 계산에서 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float EffectAmountPerStack = 0.08f;

};

// ======================= 핵심 강화 ============================

UENUM(BlueprintType)
enum class ECoreUpgradeEffectType : uint8
{
	None,
	//돌격소총 강화
	DoubleShot,
	ChainPulse,
	ExplosiveRound,
	//샷건 강화
	PiercingPellet,
	FocusedSpread,
	KnockBack,
	//레일건 강화
	MultiPierce,
	ChargeBlast,
	KillDetonation
};

USTRUCT(BlueprintType)
struct FRGCoreUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText CategoryText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText NameText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DescriptionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ValueChangeText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<ARGBaseWeapon> RequiredWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECoreUpgradeEffectType EffectType = ECoreUpgradeEffectType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DamagePercent = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxTargets = 1;
};