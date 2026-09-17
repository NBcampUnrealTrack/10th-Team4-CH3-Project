// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_EnemyMoveToPlayer.h"
#include "Enemy/AIEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_EnemyMoveToPlayer::UBTTask_EnemyMoveToPlayer()
{
	NodeName = "Move To Target";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyMoveToPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	EnemyCont = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TEXT("Target")));
	if (!Target || !EnemyCont)
	{
		return EBTNodeResult::Failed;
	}
	return EBTNodeResult::InProgress;
}


void UBTTask_EnemyMoveToPlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	EEnemyStateEnum CurrentState = (EEnemyStateEnum)BlackboardComp->GetValueAsEnum(TEXT("EnemyState"));
	if (CurrentState != EEnemyStateEnum::Chase)
	{
		EnemyCont->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	TimeSinceLastMove += DeltaSeconds;

	if (Target && TimeSinceLastMove>= MoveUpdateInterval)
	{
		TimeSinceLastMove = 0.0f;
		FVector CurrentTargetLoc = Target->GetActorLocation();
		CurrentTargetLoc.Z = EnemyCont->GetPawn()->GetActorLocation().Z;
		EnemyCont->MoveToLocation(CurrentTargetLoc);
	}
}