// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemySpawnPoint.h"

// Sets default values
AEnemySpawnPoint::AEnemySpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

bool AEnemySpawnPoint::CanBeUsedForSpawn() const
{
	return false;
}

bool AEnemySpawnPoint::CanBeUsedForRecovery() const
{
	return false;
}

FTransform AEnemySpawnPoint::GetEnemySpawnTransform(TSubclassOf<ABaseEnemy> EnemyClass) const
{
	return FTransform();
}

bool AEnemySpawnPoint::PassesSpawnDistanceCheck(const AActor* PlayerActor) const
{
	return false;
}

