// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_FaceTarget.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API UBTS_FaceTarget : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTS_FaceTarget();
	UPROPERTY()
	float RotationSpeed = 10.0f;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
