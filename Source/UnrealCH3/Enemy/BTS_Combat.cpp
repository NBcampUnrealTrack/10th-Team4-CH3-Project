// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BTS_Combat.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Enemy/BaseEnemy.h"

UBTS_Combat::UBTS_Combat()
{
	NodeName = "Update Combat";
}


void UBTS_Combat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AActor* TargetObj = Cast<AActor>(BlackboardComp->GetValueAsObject(TEXT("Target")));

	AAIController* AiComp = OwnerComp.GetAIOwner();
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(AiComp->GetPawn());
	if (!TargetObj)
	{
		OwnerComp.GetBlackboardComponent()->SetValueAsBool(TEXT("IsCombat"), false);
		return;
	}
	float Distance = FVector::Distance(Enemy->GetActorLocation(), TargetObj->GetActorLocation());
	AttackDistance = Enemy->GetViewingDistance();
	if (Distance <= AttackDistance)
	{
		OwnerComp.GetBlackboardComponent()->SetValueAsBool(TEXT("IsCombat"), true);
	}
	else
	{
		OwnerComp.GetBlackboardComponent()->SetValueAsBool(TEXT("IsCombat"), false);
	}
}