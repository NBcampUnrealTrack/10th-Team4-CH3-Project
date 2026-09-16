// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_Attack.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AIEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = "Attack";
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AAIEnemyController* EnemyCont = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(Blackboard->GetValueAsObject(TEXT("SelfActor")));

	Enemy->Attack();
	return EBTNodeResult::Succeeded;
}