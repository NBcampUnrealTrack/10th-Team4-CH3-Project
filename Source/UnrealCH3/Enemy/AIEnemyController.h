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

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	virtual void OnPossess(APawn* InPawn) override;
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	UFUNCTION()
	void StopAI();
	UFUNCTION()
	void UpdateSight();

protected:
	UPROPERTY(VisibleAnywhere, Category = "AI")
	UAIPerceptionComponent* Perception;
	class UAISenseConfig_Sight* Sight;
};
