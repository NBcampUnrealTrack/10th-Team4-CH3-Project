// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "Enemy/Enum/EnemyStateEnum.h"
#include "EnemyBlackboardKeyType_Enum.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API UEnemyBlackboardKeyType_Enum : public UBlackboardKeyType_Enum
{
	GENERATED_BODY()

public:
	UEnemyBlackboardKeyType_Enum() 
	{
		EnumType = StaticEnum<EEnemyStateEnum>();
	}
};
