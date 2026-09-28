// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BTTask_FindPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindPlayer::UBTTask_FindPlayer()
{
	NodeName = TEXT("Find Player Location");
}

EBTNodeResult::Type UBTTask_FindPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		AActor* targetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TEXT("Target")));

		if (targetActor)
		{
			BlackboardComp->SetValueAsVector(TEXT("TargetLocation"), targetActor->GetActorLocation());
			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}
  