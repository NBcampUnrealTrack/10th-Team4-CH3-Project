#include "Enemy/BossEnemy.h"

ABossEnemy::ABossEnemy()
{
	EnemyName = TEXT("Boss");

	AttackRangeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackRangeMeshComponent"));
	AttackRangeMesh->SetupAttachment(RootComponent);
	AttackRangeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackRangeMesh->SetCastShadow(false);
	AttackRangeMesh->SetVisibility(false);
	AttackRangeMesh->SetWorldScale3D(FVector(AttackMaxRange / 100.0f, 1.0f, 0.2f));
}

void ABossEnemy::Attack()
{

}