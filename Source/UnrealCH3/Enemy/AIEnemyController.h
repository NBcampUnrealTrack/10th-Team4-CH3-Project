// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIEnemyController.generated.h"


UCLASS()
class UNREALCH3_API AAIEnemyController : public AAIController
{

	GENERATED_BODY()
	
public:
	AAIEnemyController();
	virtual void OnPossess(APawn* InPawn) override;

	void RunAI();
	void StopAI();

	UPROPERTY(EditAnywhere, Category = "AI")
	class UBehaviorTree* BtAsset;

	UPROPERTY(EditAnywhere, Category = "AI")
	class UBlackboardData* BbAsset;

	//UPROPERTY(EditAnywhere, Category = "AI")
	UPROPERTY()
	class UBlackboardComponent* BbComp;
};
