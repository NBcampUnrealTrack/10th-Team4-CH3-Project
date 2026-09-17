// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_WarningAttack.h"
#include "Enemy/BaseEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_WarningAttack::UBTTask_WarningAttack()
{
	NodeName = "Warning";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_WarningAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	float* ElapsedTime = reinterpret_cast<float*>(NodeMemory);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	Enemy = Cast<ABaseEnemy>(Blackboard->GetValueAsObject(TEXT("SelfActor")));
	Target = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("Target")));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}
	WarningTime = Enemy->GetWarningTime();
	*ElapsedTime = 0.0f;
	Enemy->ShowAttackRangeLine();
	return EBTNodeResult::InProgress;
}

void UBTTask_WarningAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	float* ElapsedTime = reinterpret_cast<float*>(NodeMemory);
	*ElapsedTime += DeltaSeconds;
	if (*ElapsedTime >= WarningTime)
	{
		WarningTime = 0.0f;
		Enemy->HideAttackRangeLine();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}