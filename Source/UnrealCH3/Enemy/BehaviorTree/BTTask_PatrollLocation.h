// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PatrollLocation.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API UBTTask_PatrollLocation : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_PatrollLocation();

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolRadius = 1000.f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

