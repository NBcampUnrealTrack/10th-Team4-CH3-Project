// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyStateEnum.generated.h"

UENUM(BlueprintType)
enum class EEnemyStateEnum :uint8
{
    Idle    UMETA(DisplayName = "Idle"),
    Patrol  UMETA(DisplayName = "Patrol"),
    Chase   UMETA(DisplayName = "Chase"),
    Attack  UMETA(DisplayName = "Attack"),
    Hit     UMETA(DisplayName = "Hit"),
    Dead    UMETA(DisplayName = "Dead")
};
