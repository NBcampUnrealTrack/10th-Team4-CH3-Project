// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/AIEnemyController.h"
#include "Enemy/BaseEnemy.h"
#include "Player/RGCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

AAIEnemyController::AAIEnemyController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
	Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing Config"));

	Sight->SightRadius = 2200.0f;
	Sight->LoseSightRadius = 3500.0f;
	Sight->PeripheralVisionAngleDegrees = 70;

	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = false;

	Hearing->HearingRange = 2200.0f;
	Hearing->DetectionByAffiliation.bDetectEnemies = true;
	Hearing->DetectionByAffiliation.bDetectNeutrals = true;
	Hearing->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*Sight);
	Perception->ConfigureSense(*Hearing);
	Perception->SetDominantSense(*Sight->GetSenseImplementation());

	Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AAIEnemyController::OnPerceptionUpdated);
}

void AAIEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
}

void AAIEnemyController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn());
	if (!Enemy || !Actor) return;
	ARGCharacter* Player = Cast<ARGCharacter>(Actor);
	if (!Player) return;
	
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	
	if (!Stimulus.WasSuccessfullySensed())
	{
		Enemy->SetTargetActor(nullptr);
		if (BlackboardComponent)
		{
			BlackboardComponent->ClearValue(TEXT("Target"));
		}
		return;
	}


	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		Enemy->SetTargetActor(Actor);
		if (BlackboardComponent)
		{
			BlackboardComponent->SetValueAsObject(TEXT("Target"), Actor);
		}
	}
	else if(Stimulus.Type == UAISense::GetSenseID<UAISenseConfig_Hearing>())
	{
		if (BlackboardComponent)
		{
			BlackboardComponent->SetValueAsVector(TEXT("HeardLocation"), Stimulus.StimulusLocation);
			BlackboardComponent->SetValueAsBool(TEXT("bHeardNoise"), true);
		}
	}
}

void AAIEnemyController::StopAI()
{
	UBehaviorTreeComponent* behaviorTreeComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (nullptr == behaviorTreeComponent) return;
	behaviorTreeComponent->StopTree(EBTStopMode::Safe);
}

void AAIEnemyController::UpdateSight()
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn());
	if (Enemy)
	{
		Sight->SightRadius = Enemy->GetViewingDistance();
		Sight->LoseSightRadius = Enemy->GetViewingDistance() + 500.0f;
		Sight->PeripheralVisionAngleDegrees = Enemy->GetViewingAngle() / 2.0f;

		Hearing->HearingRange = Enemy->GetHearingDistance();
		Perception->ConfigureSense(*Sight);
		Perception->ConfigureSense(*Hearing);

		if (Enemy->GetEnemyBehaviorTree())
		{
			RunBehaviorTree(Enemy->GetEnemyBehaviorTree());
		}
	}
}
