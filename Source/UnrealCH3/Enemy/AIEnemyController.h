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
	UFUNCTION()
	virtual void OnPossess(APawn* InPawn) override;
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	UFUNCTION()
	void RunAI();
	UFUNCTION()
	void StopAI();

	UPROPERTY(EditAnywhere, Category = "AI")
	class UBehaviorTree* BtAsset;

	UPROPERTY(EditAnywhere, Category = "AI")
	class UBlackboardData* BbAsset;

	//UPROPERTY(EditAnywhere, Category = "AI")
	UPROPERTY()
	class UBlackboardComponent* BbComp;

protected:
	UAIPerceptionComponent* Perception;
	class UAISenseConfig_Sight* Sight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bCanAttack;        // 공격 가능 여부
	UPROPERTY()
	FVector TargetLocation;

};
