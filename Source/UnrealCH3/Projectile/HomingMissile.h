#pragma once

#include "CoreMinimal.h"
#include "Projectile/BaseProjectile.h"
#include "HomingMissile.generated.h"

struct FBossSkillRow;

UCLASS()
class UNREALCH3_API AHomingMissile : public ABaseProjectile
{
	GENERATED_BODY()
	
public:
	AHomingMissile();

protected:
	virtual void BeginPlay() override;

public:
	virtual void InitializeProjectile(const FBossSkillRow& SkillRow, AActor* TargetActor) override;
	void DirectHoming(AActor* TargetActor);
	
public:
	UPROPERTY(EditAnywhere, Category = "Homing")
	bool bIsDirectHoming = false;
};
