#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BossSkillRow.generated.h"

UENUM(BlueprintType)
enum class EBossSkillType : uint8
{
	FireProjectile	UMETA(DisplayName = "Fire Projectile"),
	ShockWave		UMETA(DisplayName = "Shock Wave"),
	HomingMissile	UMETA(DisplayNAme = "Homing Missile")
};

USTRUCT(BlueprintType)
struct FBossSkillRow : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseProjectile> ProjectileClass;
	
	UPROPERTY(EditAnywhere)
	EBossSkillType SkillType;

	UPROPERTY(EditAnywhere)
	int32 Phase;

	UPROPERTY(EditAnywhere)
	int32 SpawnCount;

	UPROPERTY(EditAnywhere)
	float SpawnAngle;

	UPROPERTY(EditAnywhere)
	float Damage;
};
