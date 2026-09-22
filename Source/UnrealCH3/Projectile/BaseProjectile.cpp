#include "Projectile/BaseProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Enemy/DataTableStruct/BossSkillRow.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SetRootComponent(SphereCollision);
	SphereCollision->SetMobility(EComponentMobility::Movable);
	SphereCollision->SetNotifyRigidBodyCollision(true);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SphereCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SphereCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	SphereCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(SphereCollision);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->InitialSpeed = 2000.0f;
	ProjectileMovementComp->MaxSpeed = 2000.0f;
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->ProjectileGravityScale = 0.0f;
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (SphereCollision)
	{
		SphereCollision->OnComponentHit.AddDynamic(this, &ABaseProjectile::OnHit);
	}
}

void ABaseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseProjectile::InitializeProjectile(const FBossSkillRow& SkillRow, AActor* TargetActor)
{
	Damage = FMath::Max(1.0f, SkillRow.Damage);
	Particle = SkillRow.HitParticle;
	Sound = SkillRow.HitSound;
}

void ABaseProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor->ActorHasTag("Player"))
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this, UDamageType::StaticClass());

		if (Sound)
		{
			UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Sound, GetActorLocation());
		}
	}

	if (Particle)
	{
		UParticleSystemComponent* SpawnParticle = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, GetActorLocation(), GetActorRotation());
		if (SpawnParticle)
		{
			FTimerHandle ParticleTimerHandle;
			GetWorldTimerManager().SetTimer(ParticleTimerHandle, FTimerDelegate::CreateLambda([SpawnParticle]() { if (IsValid(SpawnParticle)) { SpawnParticle->DestroyComponent(); }}), 2.0f, false);
		}
	}

	Destroy();
}

