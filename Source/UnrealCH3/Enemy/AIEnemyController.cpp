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
	static ConstructorHelpers::FObjectFinder<UBlackboardData> bb(TEXT("/Game/AI/BB_Enemy.BB_Enemy"));
	if (bb.Succeeded())
	{
		BbAsset = bb.Object;
	}
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> bt(TEXT("/Game/AI/BT_Enemy.BT_Enemy"));
	if (bb.Succeeded())
	{
		BtAsset = bt.Object;
	}
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
	}
	RunAI();
}

void AAIEnemyController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn());
	if (!Enemy || !Actor) return;
	ARGCharacter* Player = Cast<ARGCharacter>(Actor);
	if (!Player) return;
	if (Stimulus.WasSuccessfullySensed())
	{
		Enemy->SetTargetActor(Actor);
		BbComp->SetValueAsObject(TEXT("Target"), Actor);
	}
	else
	{
		
		Enemy->SetTargetActor(nullptr);
		BbComp->SetValueAsObject(TEXT("Target"), nullptr);
	}
}

void AAIEnemyController::RunAI()
{
	if (!BbAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("BbAsset NULL"));
	}
	if (UseBlackboard(BbAsset, BbComp))
	{
		RunBehaviorTree(BtAsset);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UseBlackboard Failed! BbAsset: %s"), BbAsset ? *BbAsset->GetName() : TEXT("NULL"));
	}
	if (!BbComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("BbComponent NULL"));
	}
}

void AAIEnemyController::StopAI()
{
	UBehaviorTreeComponent* behaviorTreeComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (nullptr == behaviorTreeComponent) return;
	behaviorTreeComponent->StopTree(EBTStopMode::Safe);
}
