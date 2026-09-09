// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/AIEnemyController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AAIEnemyController::AAIEnemyController()
{
}

void AAIEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RunAI();
}

void AAIEnemyController::RunAI()
{
	if (UseBlackboard(BbAsset, BbComp))
	{
		RunBehaviorTree(BtAsset);
	}
}

void AAIEnemyController::StopAI()
{
	UBehaviorTreeComponent* behaviorTreeComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (nullptr == behaviorTreeComponent) return;
	behaviorTreeComponent->StopTree(EBTStopMode::Safe);
}
