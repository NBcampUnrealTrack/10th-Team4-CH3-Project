#include "Enemy/BaseAreaAttack.h"
#include "Components/StaticMeshComponent.h"
#include "Enemy/DataTableStruct/BossSkillRow.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/RGCharacter.h"

ABaseAreaAttack::ABaseAreaAttack()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	WarningMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarningMesh"));
	WarningMesh->SetupAttachment(SceneRoot);
	WarningMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackMesh"));
	AttackMesh->SetupAttachment(SceneRoot);
	AttackMesh->SetVisibility(false);
}

void ABaseAreaAttack::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(AreaTimerHandle, this, &ABaseAreaAttack::ActivateAttack, WarningTime, false);
}

void ABaseAreaAttack::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AreaTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ABaseAreaAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMoving && AttackMesh)
	{
		FVector CurrentLocation = AttackMesh->GetRelativeLocation();
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetRelativeLocation, DeltaTime, AttackMeshMoveSpeed);

		AttackMesh->SetRelativeLocation(NewLocation);
		float Distance = FVector::DistSquared(NewLocation, TargetRelativeLocation);
		if (FMath::IsNearlyZero(Distance, 1.0f))
		{
			AttackMesh->SetRelativeLocation(TargetRelativeLocation);
			bIsMoving = false;
			SetActorTickEnabled(false);
		}
	}
}

void ABaseAreaAttack::ActivateAttack()
{
	if (!WarningMesh) return;
	WarningMesh->SetVisibility(false);

	if (!AttackMesh) return;
	bIsMoving = true;
	AttackMesh->SetVisibility(true);
	SetActorTickEnabled(true);

	if (AttackParticle)
	{
		if (UParticleSystemComponent* SpawnParticle = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), AttackParticle, GetActorLocation(), GetActorRotation()))
		{
			FTimerHandle ParticleDestoryHandle;
			GetWorldTimerManager().SetTimer(ParticleDestoryHandle, FTimerDelegate::CreateLambda([SpawnParticle]() { if (IsValid(SpawnParticle)) { SpawnParticle->DestroyComponent(); }}), 2.0f, false);
		}
	}

	if (Target.IsValid() && TargetInArea())
	{
		UGameplayStatics::ApplyDamage(Target.Get(), Damage, nullptr, this, UDamageType::StaticClass());

		if (AttackSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(GetWorld(), AttackSound, GetActorLocation());
		}
	}
	SetLifeSpan(1.0f);
}

bool ABaseAreaAttack::TargetInArea()
{
	return false;
}

void ABaseAreaAttack::InitializeAttack(const FBossSkillRow& Row, AActor* TargetActor)
{
	Damage = Row.Damage;
	Target = Cast<ARGCharacter>(TargetActor);
	AttackParticle = Row.HitParticle;
	AttackSound = Row.HitSound;

	if (WarningMesh)
	{
		FBoxSphereBounds MeshBounds = WarningMesh->GetStaticMesh()->GetBounds();
		float Radius = MeshBounds.BoxExtent.X;
		float ScaleXY = WarningDistance / Radius;
		WarningMesh->SetWorldScale3D(FVector(ScaleXY, ScaleXY, 0.1f));
		if (AttackMesh)
		{
			AttackMesh->SetWorldScale3D(FVector(ScaleXY, ScaleXY, 1.0f));
		}
	}
}


