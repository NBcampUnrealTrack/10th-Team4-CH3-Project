#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseAreaAttack.h"
#include "ShockWaveAttack.generated.h"

UCLASS()
class UNREALCH3_API AShockWaveAttack : public ABaseAreaAttack
{
	GENERATED_BODY()
	
public:
	AShockWaveAttack();

protected:
	virtual void BeginPlay() override;
	virtual bool TargetInArea() override;
public:
	virtual void InitializeAttack(const FBossSkillRow& Row, AActor* TargetActor) override;
};
