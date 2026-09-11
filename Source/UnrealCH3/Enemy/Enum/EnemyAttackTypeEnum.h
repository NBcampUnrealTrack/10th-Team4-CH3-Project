// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyAttackTypeEnum.generated.h"

UENUM(BlueprintType)
enum class EEnemyAttackType : uint8
{
	melee	UMETA(DisplayName = "Melee"),
	Ranged	UMETA(DisplayName = "Ranged")
};