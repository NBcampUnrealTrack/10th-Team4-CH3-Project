#include "Enemy/BossEnemy.h"
#include "Engine/DataTable.h"
#include "Projectile/BaseProjectile.h"
#include "BaseAreaAttack.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/DecalComponent.h"

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

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (SkillDataTable)
	{
		TArray<FBossSkillRow*> AllRows;
		SkillDataTable->GetAllRows<FBossSkillRow>(TEXT("BossSkill"), AllRows);
		
		for (FBossSkillRow* Row : AllRows)
		{
			if (Row)
			{
				CachedSkills.Add(Row->SkillType, *Row);
			}
		}
	}
}

void ABossEnemy::Attack()
{
	Super::Attack();

	UseAttackPattern();
}

void ABossEnemy::OnChangedHealth()
{
	Super::OnChangedHealth();

	float CurrentHPRatio = CurrentHP / MaxHP;

	int32 NewPhase = CurrentPhase + 1;

	if (BossPhaseThreshold.Contains(NewPhase))
	{
		float PhaseThresholdRatio = BossPhaseThreshold[NewPhase];
		if (CurrentHPRatio <= PhaseThresholdRatio)
		{
			ChangePhase(NewPhase);
			UE_LOG(LogTemp, Error, TEXT("Change Phase!!!!"));
		}
	}
}

void ABossEnemy::UseAttackPattern()
{
	switch (CurrentPhase)
	{
	case 1:
		PhaseOnePattern();
		break;
	case 2:
		PhaseTwoPattern();
		break;
	}
}

void ABossEnemy::SpawnProjectile(EBossSkillType Type)
{
	const FBossSkillRow* Row = CachedSkills.Find(Type);
	if (!Row) return;

	int32 SpawnCount = Row->SpawnCount;
	float SpawnAngle = Row->SpawnAngle;

	float StartSpawnAngle = -SpawnAngle / 2;
	float AngleGap = SpawnCount > 1 ? SpawnAngle / (SpawnCount - 1) : 0.0f;
	
	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 100;
	FRotator BaseRotation = GetActorRotation();
	if (TargetActor)
	{
		FVector ToTargetDir = TargetActor->GetActorLocation() - GetActorLocation();
		BaseRotation = ToTargetDir.Rotation();
	}

	for (int32 i = 0; i < SpawnCount; ++i)
	{
		float NewSpawnYaw = StartSpawnAngle + (AngleGap * i);
		FRotator SpawnRotation = BaseRotation;
		SpawnRotation.Yaw += NewSpawnYaw;

		// 데미지랑 발사체 셋팅하기위해 딜레이 걸기
		ABaseProjectile* Projectile = GetWorld()->SpawnActorDeferred<ABaseProjectile>(Row->ProjectileClass, FTransform(SpawnRotation, SpawnLocation));
		if (Projectile)
		{
			Projectile->InitializeProjectile(*Row, TargetActor);
			UGameplayStatics::FinishSpawningActor(Projectile, FTransform(SpawnRotation, SpawnLocation));
		}
	}
}

void ABossEnemy::FireProjectile()
{
	ProjectileFireCount++;
	SpawnProjectile(EBossSkillType::FireProjectile);
}

void ABossEnemy::ShockWave()
{
	ProjectileFireCount = 0;
	bCanUseHoming = true;
	const FBossSkillRow* Row = CachedSkills.Find(EBossSkillType::ShockWave);
	if (!Row || !TargetActor) return;

	FVector SpawnLocation = GetGroundLocation(this);
	FRotator SpawnRotation = GetActorRotation();

	ABaseAreaAttack* Attack = GetWorld()->SpawnActorDeferred<ABaseAreaAttack>(Row->AreaAttackClass, FTransform(SpawnRotation, SpawnLocation));
	if (Attack)
	{
		Attack->InitializeAttack(*Row, TargetActor);
		UGameplayStatics::FinishSpawningActor(Attack, FTransform(SpawnRotation, SpawnLocation));
	}

	/*float ShockWaveDistance = 2000.0f;
	float ShockWaveAngle = Row->SpawnAngle;
	FVector Forward = GetActorForwardVector();
	FVector ToTargetVector = TargetActor->GetActorLocation() - GetActorLocation();

	float Distance = ToTargetVector.Size2D();
	if (Distance <= ShockWaveDistance)
	{
		FVector ToTargetDir = ToTargetVector.GetSafeNormal();
		float Dot = FVector::DotProduct(ToTargetDir, Forward);

		if (Dot >= FMath::Cos(FMath::DegreesToRadians(ShockWaveAngle)))
		{
			if (TargetActor && TargetActor->ActorHasTag(TEXT("Player")))
			{
				UGameplayStatics::ApplyDamage(TargetActor, Row->Damage, GetController(), this, UDamageType::StaticClass());
				UE_LOG(LogTemp, Warning, TEXT("ShockWave"));
			}
		}
	}

	DrawDebugCone(
		GetWorld(),
		GetActorLocation(),
		GetActorForwardVector(),
		ShockWaveDistance,
		FMath::DegreesToRadians(ShockWaveAngle),
		FMath::DegreesToRadians(0.0f),
		32,
		FColor::Red,
		false,
		2.0f,
		0,
		1.0f
	);*/
}

void ABossEnemy::HomingMissile()
{
	bCanUseHoming = false;
	SpawnProjectile(EBossSkillType::HomingMissile);
}

void ABossEnemy::RiseSpike()
{
	const FBossSkillRow* Row = CachedSkills.Find(EBossSkillType::RiseSpike);
	if (!Row || !TargetActor) return;

	FVector SpawnLocation = GetGroundLocation(TargetActor);
	FRotator SpawnRotation = TargetActor->GetActorRotation();

	ABaseAreaAttack* Attack = GetWorld()->SpawnActorDeferred<ABaseAreaAttack>(Row->AreaAttackClass, FTransform(SpawnRotation, SpawnLocation));
	if (Attack)
	{
		Attack->InitializeAttack(*Row, TargetActor);
		UGameplayStatics::FinishSpawningActor(Attack, FTransform(SpawnRotation, SpawnLocation));
	}
}

void ABossEnemy::PhaseOnePattern()
{
	if (ProjectileFireCount < 5)
	{
		FireProjectile();
	}
	else
	{
		ShockWave();
	}
}

void ABossEnemy::PhaseTwoPattern()
{
	if (ProjectileFireCount == 3)
	{
		ProjectileFireCount++;
		RiseSpike();
		return;
	}

	if (bCanUseHoming)
	{
		HomingMissile();
	}
	else
	{
		PhaseOnePattern();
	}
}

void ABossEnemy::ChangePhase(int32 NewPhase)
{
	CurrentPhase = NewPhase;
	ProjectileFireCount = 0;
}

FVector ABossEnemy::GetGroundLocation(AActor* Actor)
{
	if (!Actor) return FVector::ZeroVector;

	FVector Start = Actor->GetActorLocation();
	FVector End = Start - FVector(0.0f, 0.0f, 2000.0f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Actor);
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_Visibility, Params);
	if (bHit)
	{
		return Hit.ImpactPoint;
	}

	return Start;
}

