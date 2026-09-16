#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RGEnemySpawnData.generated.h"

class ABaseEnemy;

// 초기 생성 스포너 구조체
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemyInitialSpawnGroup
{
	GENERATED_BODY()
	// 적 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<ABaseEnemy> EnemyClass;
	// 스폰 카운터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	int32 Count = 0;
	// 엘리트 몹 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bElite = false;
};

// 보충 생성 스포너 구조체
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemyWeightedSpawnEntry
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<ABaseEnemy> EnemyClass;

	//스폰 우선치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float SpawnWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bElite = false;
};

// 스폰 예산 구조체
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemySpawnBudgetRow : public FTableRowBase
{
	GENERATED_BODY()

	//동시에 살아있는 적 수 + 엘리트의 최대 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget", meta = (ClampMin = "1"))
	int32 MaxConcurrentEnemies = 5;

	//보스전 적 생성 제한 해제용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget")
	bool bUnlimitedTotalEnemyBudget = false;

	//맵 최대 생성 제한 수(일반 + 엘리트)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget",
		meta = (
			ClampMin = "0",
			EditCondition = "!bUnlimitedTotalEnemyBudget",
			EditConditionHides))
	int32 TotalEnemyBudget = 12;

	//한 맵에서 최대 엘리트 스폰 상한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget", meta = (ClampMin = "0"))
	int32 TotalEliteSpawnLimit = 0;

	//스포너 첫 생성 구성 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial Spawn")
	TArray<FEnemyInitialSpawnGroup> InitialComposition;

	//보충 시 무작위 후보 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refill")
	TArray<FEnemyWeightedSpawnEntry> RefillCandidates;

	//보충 간견
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refill", meta = (ClampMin = "0.01", Units = "s"))
	float RefillIntervalSeconds = 3.0f;

	//보충 방식 (전부(true) or 하나씩(false))
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refill")
	bool bFillToConcurrentLimitOnRefill = true;
};

//ai 이동 안전규칙 (낙사 방지)
UENUM(BlueprintType)
enum class EEnemyNavigationSaftyRule : uint8
{
	LedgeProbe          UMETA(DisplayName = "Ledge Probe"), // 플랫폼 가장자리 탐지
	JumpLink            UMETA(DisplayName = "Jump Link"), // 플랫폼 간 점프 연결
	LandingPrediction   UMETA(DisplayName = "Landing Prediction"), // 착지 예측
	PathFailure         UMETA(DisplayName = "Path Failure"), // 경로 탐색 실패
	OffscreenReposition UMETA(DisplayName = "Offscreen Reposition") //화면 밖 위치 재조정
};

//ai 안전 규칙 실패 시 대응 
UENUM(BlueprintType)
enum class EEnemySafetyFailureAction : uint8
{
	SlowAndFindDirection   UMETA(DisplayName = "Slow And Find Direction"), //감속 후 방향 재조정
	RejectLink             UMETA(DisplayName = "Reject Link"), // 이동 중지
	SelectNearestSafePoint UMETA(DisplayName = "Select Nearest Safe Point"), // 가까운 안전지대 찾기
	BlockAttackAndRecover  UMETA(DisplayName = "Block Attack And Recover"), // 공격 중지 후 복구
	RepositionOffscreen    UMETA(DisplayName = "Reposition Offscreen") // 화면 밖에서 위치 재조정
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemyNavigationSafetyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	bool bEnabled = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	EEnemyNavigationSaftyRule Rule = EEnemyNavigationSaftyRule::LedgeProbe;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	EEnemySafetyFailureAction FailureAction = EEnemySafetyFailureAction::SlowAndFindDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probe", meta = (ClampMin = "0.0",Units = "cm"))	
	float ForwardProbeDistance = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probe", meta = (ClampMin = "0.0",Units = "cm"))	
	float DownProbeDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction", meta = (ClampMin = "0.0", Units = "s"))
	float PredictionTimeSeconds = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float FailureDurationSeconds = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float RecheckIntervalSeconds = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float CooldownSeconds = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	bool bRequireDestinationNavMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	bool bCheckBeforeLinkEntry = false;




};	