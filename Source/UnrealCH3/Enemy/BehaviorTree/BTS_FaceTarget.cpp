// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTS_FaceTarget.h"
#include "Enemy/BaseEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTS_FaceTarget::UBTS_FaceTarget()
{
	NodeName = TEXT("Face Target");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTS_FaceTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	ABaseEnemy* Enemy = Cast<ABaseEnemy>(Blackboard->GetValueAsObject(TEXT("SelfActor")));
	if (Enemy)
	{
		Enemy->FaceTarget(DeltaSeconds, RotationSpeed);
	}
}