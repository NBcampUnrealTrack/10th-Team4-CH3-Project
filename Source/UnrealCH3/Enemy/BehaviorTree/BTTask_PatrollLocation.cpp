// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_PatrollLocation.h"
#include "Enemy/BaseEnemy.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_PatrollLocation::UBTTask_PatrollLocation()
{
	NodeName = TEXT("Find Patrol Location");
}

EBTNodeResult::Type UBTTask_PatrollLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("[Patrol] ExecuteTask START"));
	Super::ExecuteTask(OwnerComp, NodeMemory);
	TObjectPtr<UBlackboardComponent> BB = OwnerComp.GetBlackboardComponent();
	TObjectPtr<ABaseEnemy> Enemy = Cast<ABaseEnemy>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy None"));
		return EBTNodeResult::Failed;
	}
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(Enemy->GetWorld());
	if (!NavSystem)
	{
		UE_LOG(LogTemp, Log, TEXT("Nav None"));
		return EBTNodeResult::Failed;
	}
	FVector Origin = Enemy->GetActorLocation();
	FNavLocation NextPartol;

	if (NavSystem->GetRandomPointInNavigableRadius(Origin, 500.0f, NextPartol))
	{
		BB->SetValueAsVector(TEXT("PatrolLocation"), NextPartol.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}