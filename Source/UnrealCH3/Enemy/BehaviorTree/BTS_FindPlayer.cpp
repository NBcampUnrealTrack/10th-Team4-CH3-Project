// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTS_FindPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/BaseEnemy.h"
#include "AIController.h"

UBTS_FindPlayer::UBTS_FindPlayer()
{
	NodeName = "Find Player";
}

void UBTS_FindPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	
	if (!BlackboardComp) return;

	AAIController* AIController = OwnerComp.GetAIOwner();
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(AIController->GetPawn());
	AActor* TargetActor = Cast<AActor>(Enemy->GetTargetActor());
	BlackboardComp->SetValueAsObject(TEXT("Target"), TargetActor);
	
	MaxRange = Enemy->GetAttackMaxRange();
	MinRange = Enemy->GetAttackMinRange();
	
	bool bHasTarget = (TargetActor != nullptr);
	
	BlackboardComp->SetValueAsBool(TEXT("bIsPlayer"), bHasTarget);
	if (!TargetActor)
	{
		BlackboardComp->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::Patrol));
		Enemy->SetState(EEnemyStateEnum::Patrol);
		return;
	}
	
	float Distance = FVector::Dist(TargetActor->GetActorLocation(), Enemy->GetActorLocation());
	bool bInAttackRange = (Distance <= MaxRange && Distance >= MinRange);
	
	Enemy->SetEnemyTurn(true);

	if (bInAttackRange)
	{
		BlackboardComp->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::Attack));
		Enemy->SetState(EEnemyStateEnum::Attack);
	}
	else if (Distance < MinRange)
	{
		BlackboardComp->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::SoClose));
		Enemy->SetState(EEnemyStateEnum::SoClose);
		
	}
	else if (Distance >MaxRange)
	{
		BlackboardComp->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::Chase));
		Enemy->SetState(EEnemyStateEnum::Chase);
		BlackboardComp->SetValueAsVector(TEXT("TargetLocation"), TargetActor->GetActorLocation());
	}

	return;
}