// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemySpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Enemy/BaseEnemy.h"
#include "GameFramework/Character.h"

// Sets default values
AEnemySpawnPoint::AEnemySpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(SceneRoot);
	FacingArrow->SetArrowColor(FLinearColor::Green);
	FacingArrow->ArrowSize = 1.5f;
	FacingArrow->SetHiddenInGame(true);
	FacingArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FacingArrow->SetCanEverAffectNavigation(false);

	SetActorEnableCollision(false);
}

bool AEnemySpawnPoint::CanBeUsedForSpawn() const
{
	return bEnabled && bUSeForSpawn;
}

bool AEnemySpawnPoint::CanBeUsedForRecovery() const
{
	return bEnabled && bUseForRecovery;
}

FTransform AEnemySpawnPoint::GetEnemySpawnTransform(TSubclassOf<ABaseEnemy> EnemyClass) const
{
	FVector SpawnLocation = GetActorLocation();

	if (EnemyClass)
	{
		const ABaseEnemy* EnemyCDO = EnemyClass->GetDefaultObject<ABaseEnemy>();

		if (EnemyCDO && EnemyCDO->GetCapsuleComponent())
		{
			SpawnLocation.Z += EnemyCDO->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		}
	}

	SpawnLocation.Z += GroundClearance;

	const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

	return FTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
}

bool AEnemySpawnPoint::PassesSpawnDistanceCheck(const AActor* PlayerActor) const
{
	if (!bCheckMinimumPlayerDistance)
	{
		return true;
	}

	if (!PlayerActor)
	{
		return true;
	}

	const float DistanceSquared = FVector::DistSquared(GetActorLocation(), PlayerActor->GetActorLocation());

	return DistanceSquared >= FMath::Square(MinimumPlayerDistance);
}


