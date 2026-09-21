#include "Enemy/BehaviorTree/BTS_BossRader.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/AIEnemyController.h"

UBTS_BossRader::UBTS_BossRader()
{
	NodeName = TEXT("BossRader");
	bNotifyTick = true;
}

void UBTS_BossRader::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackBoardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackBoardComponent) return;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;
	ABaseEnemy* Boss = Cast<ABaseEnemy>(AIController->GetPawn());
	AActor* Target = Cast<AActor>(BlackBoardComponent->GetValueAsObject(TEXT("Target")));
	
	if (Target)
	{
		BlackBoardComponent->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::Attack));
		Boss->SetState(EEnemyStateEnum::Attack);
		BlackBoardComponent->SetValueAsVector(TEXT("TargetLocation"), Target->GetActorLocation());
		Boss->SetEnemyTurn(true);
	}
	else
	{
		BlackBoardComponent->SetValueAsEnum(TEXT("EnemyState"), static_cast<uint8>(EEnemyStateEnum::Idle));
		Boss->SetState(EEnemyStateEnum::Idle);
		Boss->SetEnemyTurn(false);
	}
}
