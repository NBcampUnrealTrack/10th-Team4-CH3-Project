// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/AIEnemyController.h"
#include "Enemy/BaseEnemy.h"
#include "Player/RGCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AAIEnemyController::AAIEnemyController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));

	Sight->SightRadius = 2200.0f;
	Sight->LoseSightRadius = 3500.0f;
	Sight->PeripheralVisionAngleDegrees = 70;

	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = false;

	Perception->ConfigureSense(*Sight);
	Perception->SetDominantSense(*Sight->GetSenseImplementation());

	Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AAIEnemyController::OnPerceptionUpdated);
}

void AAIEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(InPawn);
	if (Enemy)
	{
		Sight->SightRadius = Enemy->GetViewingDistance();
		Sight->LoseSightRadius = Enemy->GetViewingDistance() + 500.0f;
		Sight->PeripheralVisionAngleDegrees = Enemy->GetViewingAngle() / 2.0f;
		Perception->ConfigureSense(*Sight);

		if (Enemy->GetEnemyBehaviorTree())
		{
			RunBehaviorTree(Enemy->GetEnemyBehaviorTree());
		}
	}
}

void AAIEnemyController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn());
	if (!Enemy || !Actor) return;
	ARGCharacter* Player = Cast<ARGCharacter>(Actor);
	if (!Player) return;

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

	if (Stimulus.WasSuccessfullySensed())
	{
		Enemy->SetTargetActor(Actor);
		if (BlackboardComponent)
		{
			BlackboardComponent->SetValueAsObject(TEXT("Target"), Actor);
		}
	}
	else
	{
		Enemy->SetTargetActor(nullptr);
		if (BlackboardComponent)
		{
			BlackboardComponent->ClearValue(TEXT("Target"));
		}
	}
}

void AAIEnemyController::StopAI()
{
	UBehaviorTreeComponent* behaviorTreeComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (nullptr == behaviorTreeComponent) return;
	behaviorTreeComponent->StopTree(EBTStopMode::Safe);
}
