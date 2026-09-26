#include "Projectile/HomingMissile.h"
#include "GameFramework/ProjectileMovementComponent.h"

AHomingMissile::AHomingMissile()
{

}

void AHomingMissile::BeginPlay()
{
	Super::BeginPlay();

}

void AHomingMissile::InitializeProjectile(const FBossSkillRow& SkillRow, AActor* TargetActor)
{
	Super::InitializeProjectile(SkillRow, TargetActor);
	if (TargetActor && ProjectileMovementComp)
	{
		DirectHoming(TargetActor);
	}
}

void AHomingMissile::DirectHoming(AActor* TargetActor)
{
	if (bIsDirectHoming)
	{
		ProjectileMovementComp->InitialSpeed = 1000.0f;
		ProjectileMovementComp->MaxSpeed = 1000.0f;
		ProjectileMovementComp->bIsHomingProjectile = true;
		ProjectileMovementComp->HomingAccelerationMagnitude = 3000.0f;
		ProjectileMovementComp->HomingTargetComponent = TargetActor->GetRootComponent();
	}
	else
	{
		ProjectileMovementComp->InitialSpeed = 800.0f;
		ProjectileMovementComp->MaxSpeed = 800.0f;
		ProjectileMovementComp->bIsHomingProjectile = false;
		FTimerHandle HomingTimerHandle;
		TWeakObjectPtr<AActor> WeakPtr = TargetActor;
		GetWorldTimerManager().SetTimer(HomingTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, WeakPtr]()
			{
				if (WeakPtr.IsValid())
				{
					ProjectileMovementComp->bIsHomingProjectile = true;
					ProjectileMovementComp->HomingAccelerationMagnitude = 3000.0f;
					ProjectileMovementComp->HomingTargetComponent = WeakPtr.Get()->GetRootComponent();
				}
			})
			, 2.0f, false);
	}
}
