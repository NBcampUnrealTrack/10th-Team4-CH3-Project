#include "Enemy/SpikeAttack.h"
#include "Player/RGCharacter.h"

ASpikeAttack::ASpikeAttack()
{
	WarningTime = 1.0f;
	WarningDistance = 300.0f;
	AttackMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -200.0f));
	TargetRelativeLocation = FVector::ZeroVector;
}

void ASpikeAttack::BeginPlay()
{
	Super::BeginPlay();

}

bool ASpikeAttack::TargetInArea()
{
	if (Target.IsValid())
	{
		float Distance = FVector::DistSquared2D(Target->GetActorLocation(), AttackMesh->GetComponentLocation());
		if (Distance <= FMath::Square(WarningDistance))
		{
			return true;
		}
	}

	return false;
}

void ASpikeAttack::InitializeAttack(const FBossSkillRow& Row, AActor* TargetActor)
{
	Super::InitializeAttack(Row, TargetActor);
}