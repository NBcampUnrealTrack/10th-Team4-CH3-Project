#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "DataTableStruct/BossSkillRow.h"
#include "BossEnemy.generated.h"

struct FBossSkillRow;

UCLASS()
class UNREALCH3_API ABossEnemy : public ABaseEnemy
{
	GENERATED_BODY()
	
public:
	ABossEnemy();
	
public:
	virtual void BeginPlay() override;
	virtual void Attack() override;
	virtual void OnChangedHealth() override;

public:
	void UseAttackPattern();
	void SpawnProjectile(EBossSkillType Type);
	void FireProjectile();
	void ShockWave();
	void HomingMissile();
	void RiseSpike();
	void PhaseOnePattern();
	void PhaseTwoPattern();
	void ChangePhase(int32 NewPhase);
	FVector GetGroundLocation(AActor* Actor);

protected:
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> SkillDataTable;
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TMap<int32, float> BossPhaseThreshold;

protected:
	TMap<EBossSkillType, FBossSkillRow> CachedSkills;
	int32 CurrentPhase = 1;
	int32 ProjectileFireCount = 0;

private:
	bool bCanUseHoming = true;
};
