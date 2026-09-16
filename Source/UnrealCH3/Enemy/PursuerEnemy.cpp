// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/PursuerEnemy.h"
#include "Enemy/DataTableStruct/StructEnemyState.h"
#include "Enemy/DataTableStruct/StructEnemyAttackType.h"
#include "Kismet/GameplayStatics.h"

APursuerEnemy::APursuerEnemy()
{
	EnemyName = TEXT("Pursuer");

	static ConstructorHelpers::FObjectFinder<UDataTable> Data(TEXT("/Game/Enemy/EnemyData.EnemyData"));
	static ConstructorHelpers::FObjectFinder<UDataTable> AttackData(TEXT("/Game/Enemy/EnemyAttackData.EnemyAttackData"));
	if (Data.Succeeded())
	{
		EnemyData = Data.Object;
		FStructEnemyState* DataRow = EnemyData->FindRow<FStructEnemyState>(EnemyName, TEXT(""));
		MaxHP = DataRow->MaxHP;
		Defense = DataRow->Defanse;
		AttackDamage = DataRow->Attack;
		Exp = DataRow->EXP;
		Score = DataRow->Score;
	}
	if (AttackData.Succeeded())
	{
		EnemyAttackData = AttackData.Object;
		FStructEnemyAttackType* AttackDataRow = EnemyAttackData->FindRow<FStructEnemyAttackType>(EnemyName, TEXT(""));

		ViewingDistance = AttackDataRow->ViewingDistance;
		ViewingAngle = AttackDataRow->ViewingAngle;
		HearingDistance = AttackDataRow->HearingDistance;
		TargetChangeTime = AttackDataRow->TargetChangeTime;

		AttackMinRange = AttackDataRow->MinAttackRange;
		AttackMaxRange = AttackDataRow->MaxAttackRange;
		AttackCoolTime = AttackDataRow->CoolTime;
		WarningTime = AttackDataRow->WarningTime;

		AttackType = AttackDataRow->AttackType;
	}

	AttackRangeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackRangeMeshComponent"));
	AttackRangeMesh->SetupAttachment(RootComponent);
	AttackRangeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackRangeMesh->SetCastShadow(false);
	AttackRangeMesh->SetVisibility(false);
	AttackRangeMesh->SetWorldScale3D(FVector(AttackMaxRange / 100.0f, 1.0f, 0.2f));
}

void APursuerEnemy::Attack()
{
	if (!TargetActor) return; // 널 체크도 없었네요

	FVector Start = GetActorLocation() + FVector(0, 0, 50.f);
	FVector End = TargetActor->GetActorLocation(); // Forward가 아니라 타겟 위치로 직접

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
