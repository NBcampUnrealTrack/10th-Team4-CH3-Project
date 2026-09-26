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
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

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

	/** 현재 보스맵에서 아직 파괴되지 않은 코어 수 */
	UFUNCTION(BlueprintPure, Category = "Boss|Core Shield")
	int32 GetRemainingBossCoreCount() const;

	/** 현재 코어 보호막이 적용한 최종 피해 배율. 1.0이면 피해 감소 없음 */
	UFUNCTION(BlueprintPure, Category = "Boss|Core Shield")
	float GetCurrentCoreShieldDamageMultiplier() const;

protected:
	/** 보스맵의 코어 Actor에 붙일 태그. 기본값: DataCore */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Core Shield")
	FName BossCoreActorTag = TEXT("DataCore");

	/** 남아 있는 코어 1개당 피해 감소 비율. 0.20 = 코어 1개당 20% 감소 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Core Shield", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageReductionPerRemainingCore = 0.20f;

	/** 코어가 남아 있어도 최소한 이 비율의 피해는 받는다. 0.10 = 최소 10% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Core Shield", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumDamageMultiplierWhileCoreAlive = 0.10f;

	/** BeginPlay 때 태그로 검색한 보스맵 코어 총수 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Core Shield")
	int32 InitialBossCoreCount = 0;

	void CacheInitialBossCoreCount();

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
