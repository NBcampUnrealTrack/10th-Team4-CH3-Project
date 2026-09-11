// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/AIEnemyController.h"
#include "Enemy/BaseEnemy.h"
#include "Player/RGCharacter.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"


AAIEnemyController::AAIEnemyController()
{
	static ConstructorHelpers::FObjectFinder<UBlackboardData> BlackboardFinder(
		TEXT("/Game/AI/BB_Enemy.BB_Enemy")
	);

	if (BlackboardFinder.Succeeded())
	{
		BbAsset = BlackboardFinder.Object;
	}
	else
	{
		BbAsset = nullptr;

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AIEnemyController] BB_Enemy not found.")
		);
	}


	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BehaviorTreeFinder(
		TEXT("/Game/AI/BT_Enemy.BT_Enemy")
	);

	if (BehaviorTreeFinder.Succeeded())
	{
		BtAsset = BehaviorTreeFinder.Object;
	}
	else
	{
		BtAsset = nullptr;

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AIEnemyController] BT_Enemy not found.")
		);
	}


	Perception =
		CreateDefaultSubobject<UAIPerceptionComponent>(
			TEXT("AIPerceptionComponent")
		);

	Sight =
		CreateDefaultSubobject<UAISenseConfig_Sight>(
			TEXT("Sight Config")
		);


	if (Sight && Perception)
	{
		Sight->SightRadius = 2200.0f;
		Sight->LoseSightRadius = 3500.0f;
		Sight->PeripheralVisionAngleDegrees = 70.0f;

		Sight->DetectionByAffiliation.bDetectEnemies = true;
		Sight->DetectionByAffiliation.bDetectNeutrals = true;
		Sight->DetectionByAffiliation.bDetectFriendlies = false;

		Perception->ConfigureSense(*Sight);

		Perception->SetDominantSense(
			Sight->GetSenseImplementation()
		);

		Perception->OnTargetPerceptionUpdated.AddDynamic(
			this,
			&AAIEnemyController::OnPerceptionUpdated
		);
	}
}


void AAIEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);


	ABaseEnemy* Enemy =
		Cast<ABaseEnemy>(InPawn);

	if (!Enemy)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] Possessed pawn is not BaseEnemy."
			)
		);

		return;
	}


	if (!Sight || !Perception)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] Perception components are missing."
			)
		);

		return;
	}


	Sight->SightRadius =
		FMath::Max(
			0.0f,
			Enemy->GetViewingDistance()
		);

	Sight->LoseSightRadius =
		Sight->SightRadius + 500.0f;

	Sight->PeripheralVisionAngleDegrees =
		FMath::Clamp(
			Enemy->GetViewingAngle() / 2.0f,
			0.0f,
			180.0f
		);


	// 런타임 값 변경 후 다시 적용
	Perception->ConfigureSense(*Sight);


	RunAI();
}


void AAIEnemyController::OnPerceptionUpdated(
	AActor* Actor,
	FAIStimulus Stimulus
)
{
	ABaseEnemy* Enemy =
		Cast<ABaseEnemy>(GetPawn());

	if (!Enemy || !Actor)
	{
		return;
	}


	ARGCharacter* Player =
		Cast<ARGCharacter>(Actor);

	if (!Player)
	{
		return;
	}


	if (!BbComp)
	{
		return;
	}


	if (Stimulus.WasSuccessfullySensed())
	{
		Enemy->SetTargetActor(Actor);

		BbComp->SetValueAsObject(
			TEXT("Target"),
			Actor
		);
	}
	else
	{
		Enemy->SetTargetActor(nullptr);

		BbComp->ClearValue(
			TEXT("Target")
		);
	}
}


void AAIEnemyController::RunAI()
{
	if (!BbAsset)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] Blackboard asset is null."
			)
		);

		return;
	}


	if (!BtAsset)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] BehaviorTree asset is null."
			)
		);

		return;
	}


	if (!UseBlackboard(BbAsset, BbComp))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] UseBlackboard failed."
			)
		);

		return;
	}


	if (!BbComp)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] BlackboardComponent is null."
			)
		);

		return;
	}


	if (!RunBehaviorTree(BtAsset))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[AIEnemyController] RunBehaviorTree failed."
			)
		);
	}
}


void AAIEnemyController::StopAI()
{
	UBehaviorTreeComponent* BehaviorTreeComponent =
		Cast<UBehaviorTreeComponent>(
			BrainComponent
		);

	if (!BehaviorTreeComponent)
	{
		return;
	}


	BehaviorTreeComponent->StopTree(
		EBTStopMode::Safe
	);
}
