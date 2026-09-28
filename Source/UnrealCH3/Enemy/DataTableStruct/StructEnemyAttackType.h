// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Enemy/Enum/EnemyAttackTypeEnum.h"
#include "StructEnemyAttackType.generated.h"

USTRUCT(BlueprintType)
struct FStructEnemyAttackType : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EnemyName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ViewingDistance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ViewingAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HearingDistance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetChangeTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEnemyAttackType AttackType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinAttackRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAttackRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WarningTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CoolTime;
};