#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseAreaAttack.generated.h"

class ARGCharacter;
struct FBossSkillRow;

UCLASS()
class UNREALCH3_API ABaseAreaAttack : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseAreaAttack();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
public:	
	void ActivateAttack();
	virtual bool TargetInArea();
	virtual void InitializeAttack(const FBossSkillRow& Row, AActor* TargetActor);
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> WarningMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> AttackMesh;

	UPROPERTY(EditAnywhere, Category = "Effect")
	TObjectPtr<UParticleSystem> AttackParticle;

	UPROPERTY(EditAnywhere, Category = "Effect")
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(EditAnywhere, Category = "Warning", meta = (ClampMin = "1.0"))
	float WarningTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Warning")
	float WarningDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Warning")
	float WarningAngle = 0.0f;

	UPROPERTY(EditAnywhere, Category = "AttackMove", meta = (ClampMin = "1.0"))
	float AttackMeshMoveSpeed = 10.0f;

	FTimerHandle AreaTimerHandle;
	bool bIsMoving = false;
	FVector TargetRelativeLocation = FVector::ZeroVector;
	TWeakObjectPtr<ARGCharacter> Target = nullptr;
	float Damage = 0.0f;
};
