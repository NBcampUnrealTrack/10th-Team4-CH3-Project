#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseAreaAttack.h"
#include "SpikeAttack.generated.h"

UCLASS()
class UNREALCH3_API ASpikeAttack : public ABaseAreaAttack
{
	GENERATED_BODY()
	
public:
	ASpikeAttack();

protected:
	virtual void BeginPlay() override;
	virtual bool TargetInArea() override;
public:
	virtual void InitializeAttack(const FBossSkillRow& Row, AActor* TargetActor) override;
};
