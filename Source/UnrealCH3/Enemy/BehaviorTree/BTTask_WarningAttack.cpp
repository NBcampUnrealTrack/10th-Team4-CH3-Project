// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_WarningAttack.h"
#include "Enemy/BaseEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_WarningAttack::UBTTask_WarningAttack()
{
	NodeName = "Warning";
	bNotifyTick = true;
}

uint16 UBTTask_WarningAttack::GetInstanceMemorySize() const
{
	return sizeof(float); // ElapsedTime 하나만 저장
}

EBTNodeResult::Type UBTTask_WarningAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	float* ElapsedTime = reinterpret_cast<float*>(NodeMemory);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	Enemy = Cast<ABaseEnemy>(Blackboard->GetValueAsObject(TEXT("SelfActor")));
	if (!Enemy)
	{
		return EBTNodeResult::Failed;
	}
	Target = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("Target"))); 
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}
	WarningTime = Enemy->GetWarningTime();
	Enemy->ShowAttackRangeLine();
	*ElapsedTime = 0.0f;
	return EBTNodeResult::InProgress;
}

void UBTTask_WarningAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	float* ElapsedTime = reinterpret_cast<float*>(NodeMemory);

	if (!Enemy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	*ElapsedTime += DeltaSeconds;
	const float Alpha = *ElapsedTime / WarningTime;
	Enemy->UpdateAttackWarningTransform();
	Enemy->UpdateAttackWarning(Alpha);

	if (*ElapsedTime >= WarningTime)
	{
		WarningTime = 0;
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}