// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_MoveAwayFromTarget.h"
#include "Enemy/BaseEnemy.h"
#include "AIController.h"
#include "BehaviorTree/BTTaskNode.h"

UBTTask_MoveAwayFromTarget::UBTTask_MoveAwayFromTarget()
{
	NodeName = "Move Away From Target(Backstep)";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveAwayFromTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	AAIController* EnemyController = OwnerComp.GetAIOwner();
	EnemyController->StopMovement();
	
	return EBTNodeResult::InProgress;
}

void UBTTask_MoveAwayFromTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* EnemyController = OwnerComp.GetAIOwner();
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(EnemyController->GetPawn());
	AActor* Target = Enemy->GetTargetActor();
	EnemyController->StopMovement();
	Enemy->SetEnemyTurn(false);
	if (!Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	float Dist = FVector::Distance(Target->GetActorLocation(), Enemy->GetActorLocation());
	float MinRange = Enemy->GetAttackMinRange();
	if (Dist >= MinRange)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	Enemy->MoveAwayFromTarget(DeltaSeconds);
}