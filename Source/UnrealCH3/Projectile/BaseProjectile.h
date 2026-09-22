#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectile.generated.h"

struct FBossSkillRow;

UCLASS()
class UNREALCH3_API ABaseProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseProjectile();

protected:
	virtual void BeginPlay() override;	
	virtual void Tick(float DeltaTime) override;

public:
	virtual void InitializeProjectile(const FBossSkillRow& SkillRow, AActor* TargetActor);

public:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class USphereComponent> SphereCollision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovementComp;
	UPROPERTY(EditAnywhere, Category = "Effect")
	TObjectPtr<class UParticleSystem> Particle;
	UPROPERTY(EditAnywhere, Category = "Effect")
	TObjectPtr<class USoundBase> Sound;
	UPROPERTY(EditAnywhere, Category = "Damage")
	float Damage = 0.0f;
};
