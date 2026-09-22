// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BehaviorTree/BTTask_HearNoise.h"
#include "Enemy/AIEnemyController.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/Enum/EnemyStateEnum.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_HearNoise::UBTTask_HearNoise()
{
	NodeName = "Hear Noise(Turn)";
	bNotifyTick = true;
	ListenDuration = 1.5f;
}

uint16 UBTTask_HearNoise::GetInstanceMemorySize() const
{
	return sizeof(float);
}

EBTNodeResult::Type UBTTask_HearNoise::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	float* ElapsedTime = reinterpret_cast<float*>(NodeMemory);
	*ElapsedTime = 0.0f;
	return EBTNodeResult::InProgress;
}

void UBTTask_HearNoise::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	float* ElapsedTime = reinterpret_cast<float*>(NodeMemory);
	*ElapsedTime += DeltaSeconds;

	AAIEnemyController* EnemyCont = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(EnemyCont->GetPawn());
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Enemy || !Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector HearLoc = Blackboard->GetValueAsVector(TEXT("HeardLocation"));
	Enemy->FaceLocation(HearLoc, DeltaSeconds);
	if (*ElapsedTime >= ListenDuration)
	{
		Blackboard->SetValueAsBool(TEXT("bHeardNoise"), false);
		if (Blackboard->GetValueAsBool(TEXT("bIsPlayer")))
		{
			Blackboard->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::Chase));
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
