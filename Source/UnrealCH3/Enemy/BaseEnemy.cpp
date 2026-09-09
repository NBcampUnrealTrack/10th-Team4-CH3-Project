// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BaseEnemy.h"
#include "Enemy/AIEnemyController.h"

// Sets default values
ABaseEnemy::ABaseEnemy()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AAIEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();

}


// Called to bind functionality to input
void ABaseEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}



float ABaseEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);
	UE_LOG(LogTemp, Log, TEXT("Enemy Dereased to : %f"), CurrentHP);
	if (CurrentHP <= 0.0f)
	{
		Die();
	}
	return ActualDamage;
}

void ABaseEnemy::Die()
{
	UE_LOG(LogTemp, Log, TEXT("Character is Dead!"));
}

bool ABaseEnemy::IsTargetInAttackRange() const
{
	return false;
}

EEnemyState ABaseEnemy::GetEnemyState() const
{
	return EEnemyState();
}

void ABaseEnemy::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

float ABaseEnemy::GetViewingAngle()
{
	return ViewingAngle;
}

float ABaseEnemy::GetViewingDistance()
{
	return ViewingDistance;
}