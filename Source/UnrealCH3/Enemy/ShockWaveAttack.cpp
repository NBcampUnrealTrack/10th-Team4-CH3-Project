#include "Enemy/ShockWaveAttack.h"
#include "Enemy/DataTableStruct/BossSkillRow.h"
#include "Player/RGCharacter.h"

AShockWaveAttack::AShockWaveAttack()
{
	WarningTime = 1.5f;
	WarningDistance = 2000.0f;
	AttackMesh->SetRelativeLocation(FVector(-200.0f, 0.0f, 0.0f));
	TargetRelativeLocation = FVector::ZeroVector;
}

void AShockWaveAttack::BeginPlay()
{
	Super::BeginPlay();

}

bool AShockWaveAttack::TargetInArea()
{
	if (Target.IsValid())
	{
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		if (ToTarget.Size2D() <= WarningDistance)
		{
			float Dot = FVector::DotProduct(ToTarget.GetSafeNormal(), GetActorForwardVector());
			if (Dot >= FMath::Cos(FMath::DegreesToRadians(WarningAngle)))
			{
				return true;
			}
		}
	}

	return false;
}

void AShockWaveAttack::InitializeAttack(const FBossSkillRow& Row, AActor* TargetActor)
{
	Super::InitializeAttack(Row, TargetActor);

	WarningAngle = Row.SpawnAngle;
	UMaterialInstanceDynamic* MID = WarningMesh->CreateAndSetMaterialInstanceDynamic(0);
	if (MID)
	{
		MID->SetScalarParameterValue(TEXT("AngleCos"), FMath::Cos(FMath::DegreesToRadians(WarningAngle / 2.0f)));
	}
}
