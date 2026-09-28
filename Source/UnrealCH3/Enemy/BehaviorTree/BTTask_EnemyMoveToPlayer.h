// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "Enemy/Enum/EnemyStateEnum.h"
#include "BTTask_EnemyMoveToPlayer.generated.h"


UCLASS()
class UNREALCH3_API UBTTask_EnemyMoveToPlayer : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_EnemyMoveToPlayer();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	float MoveUpdateInterval = 0.2f;
	float TimeSinceLastMove = 0.0f;

	class AAIEnemyController* EnemyCont;
	class UBlackboardComponent* BlackboardComp;
	AActor* Target;
};
