// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ShooterEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"

AShooterEnemy::AShooterEnemy()
{
	EnemyName = TEXT("Shooter");

	AttackRangeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackRangeMeshComponent"));
	AttackRangeMesh->SetupAttachment(RootComponent);
	AttackRangeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackRangeMesh->SetCastShadow(false);
	AttackRangeMesh->SetVisibility(false);
	AttackRangeMesh->SetWorldScale3D(FVector(AttackMaxRange / 100.0f, 1.0f, 0.2f));
	AttackRangeMesh->SetUsingAbsoluteRotation(true); 
	AttackRangeMesh->SetUsingAbsoluteLocation(false);
}

void AShooterEnemy::Attack()
{
	if (!TargetActor) return; // 널 체크도 없었네요
	
	FVector Start = GetActorLocation() + FVector(0, 0, 50.f);
	FVector End = AttackLocation;// Forward가 아니라 타겟 위치로 직접

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);
	DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 2.0f, 0, 2.f);

	UE_LOG(LogTemp, Warning, TEXT("Attack Trace - Hit: %d, Actor: %s"),
		bHit, Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None"));

	if (bHit && Hit.GetActor() == TargetActor)
	{
		UGameplayStatics::ApplyDamage(Hit.GetActor(), AttackDamage, GetController(), this, UDamageType::StaticClass());
	}
}
