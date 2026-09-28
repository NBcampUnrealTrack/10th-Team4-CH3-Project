#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RGEnemySpawnData.generated.h"

// DataTable에서 적 클래스를 선택할 때 사용할 전방 선언
class ABaseEnemy;

// 초기 생성 스포너 구조체
// 스테이지가 처음 시작될 때 어떤 적을 몇 마리 생성할지 저장한다.
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemyInitialSpawnGroup
{
	GENERATED_BODY()
	// 적 종류
	// 초기 생성에 사용할 ABaseEnemy 기반 Blueprint 또는 C++ 클래스를 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<ABaseEnemy> EnemyClass;
	// 스폰 카운터
	// EnemyClass에 지정된 적을 초기에 생성할 수량이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	int32 Count = 0;
	// 엘리트 몹 여부
	// 해당 적이 엘리트 생성 예산을 사용하는지 구분한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bElite = false;
};

// 보충 생성 스포너 구조체
// 적이 사망하여 빈자리가 생겼을 때 생성할 후보와 선택 가중치를 저장한다.
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemyWeightedSpawnEntry
{
	GENERATED_BODY()


	// 보충 생성 후보로 사용할 ABaseEnemy 기반 클래스를 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<ABaseEnemy> EnemyClass;

	//스폰 우선치
	// 다른 후보와 비교하여 이 적이 선택될 상대적인 확률 비중이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float SpawnWeight = 1.0f;

	// 보충 후보가 엘리트 생성 예산을 사용하는 적인지 구분한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bElite = false;
};

// 스폰 예산 구조체
// DataTable의 한 행으로 사용되며 스테이지별 생성 규칙 전체를 저장한다.
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemySpawnBudgetRow : public FTableRowBase
{
	GENERATED_BODY()

	//동시에 살아있는 적 수 + 엘리트의 최대 수
	// 일반 적과 엘리트를 합산한 현재 활성 적의 최대 수이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget", meta = (ClampMin = "1"))
	int32 MaxConcurrentEnemies = 5;

	//보스전 적 생성 제한 해제용
	// true이면 TotalEnemyBudget을 무시하고 스폰 시스템이 중지될 때까지 계속 보충한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget")
	bool bUnlimitedTotalEnemyBudget = false;

	//맵 최대 생성 제한 수(일반 + 엘리트)
	// 초기 생성과 보충 생성을 모두 합산한 스테이지의 누적 생성 상한이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget",
		meta = (
			ClampMin = "0",
			EditCondition = "!bUnlimitedTotalEnemyBudget",
			EditConditionHides))
	int32 TotalEnemyBudget = 12;

	//한 맵에서 최대 엘리트 스폰 상한
	// 동시에 존재하는 엘리트 수가 아니라 스테이지에서 생성된 누적 엘리트 수를 제한한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget", meta = (ClampMin = "0"))
	int32 TotalEliteSpawnLimit = 0;

	//스포너 첫 생성 구성 데이터
	// 스폰 매니저가 최초로 시작될 때 생성할 적 클래스와 수량 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial Spawn")
	TArray<FEnemyInitialSpawnGroup> InitialComposition;

	//보충 시 무작위 후보 가중치
	// 빈자리를 보충할 때 선택할 적 클래스와 가중치 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refill")
	TArray<FEnemyWeightedSpawnEntry> RefillCandidates;

	//보충 간견
	// 스폰 매니저가 빈자리를 확인하고 보충 생성을 시도하는 시간 간격이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refill", meta = (ClampMin = "0.01", Units = "s"))
	float RefillIntervalSeconds = 3.0f;

	//보충 방식 (전부(true) or 하나씩(false))
	// true이면 한 번의 보충 주기에 동시 최대 수까지 채우고 false이면 한 마리만 생성한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refill")
	bool bFillToConcurrentLimitOnRefill = true;
};

//ai 이동 안전규칙 (낙사 방지)
// 내비게이션 안전 시스템에서 수행할 검사 종류를 구분한다.
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
// 안전 검사에 실패했을 때 AI가 수행할 대응 방식을 구분한다.
UENUM(BlueprintType)
enum class EEnemySafetyFailureAction : uint8
{
	SlowAndFindDirection   UMETA(DisplayName = "Slow And Find Direction"), //감속 후 방향 재조정
	RejectLink             UMETA(DisplayName = "Reject Link"), // 이동 중지
	SelectNearestSafePoint UMETA(DisplayName = "Select Nearest Safe Point"), // 가까운 안전지대 찾기
	BlockAttackAndRecover  UMETA(DisplayName = "Block Attack And Recover"), // 공격 중지 후 복구
	RepositionOffscreen    UMETA(DisplayName = "Reposition Offscreen") // 화면 밖에서 위치 재조정
};

// 내비게이션 안전 DataTable의 한 행으로 사용되는 설정 구조체
USTRUCT(BlueprintType)
struct UNREALCH3_API FEnemyNavigationSafetyRow : public FTableRowBase
{
	GENERATED_BODY()

	// 해당 안전 규칙을 실제로 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	bool bEnabled = true;

	// 적용할 내비게이션 안전 검사 종류를 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	EEnemyNavigationSaftyRule Rule = EEnemyNavigationSaftyRule::LedgeProbe;

	// 안전 검사에 실패했을 때 실행할 대응 방식을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	EEnemySafetyFailureAction FailureAction = EEnemySafetyFailureAction::SlowAndFindDirection;

	// 적의 현재 위치에서 전방으로 탐색할 거리이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probe", meta = (ClampMin = "0.0", Units = "cm"))
	float ForwardProbeDistance = 0.0f;

	// 전방 탐색 지점에서 바닥을 찾기 위해 아래 방향으로 탐색할 거리이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probe", meta = (ClampMin = "0.0", Units = "cm"))
	float DownProbeDistance = 0.0f;

	// 현재 속도를 기준으로 미래 위치 또는 착지 위치를 예측할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction", meta = (ClampMin = "0.0", Units = "s"))
	float PredictionTimeSeconds = 0.0f;

	// 안전 검사 실패 상태가 유지되었다고 판단할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float FailureDurationSeconds = 0.0f;

	// 실패 상태 또는 주변 안전 여부를 다시 검사하는 시간 간격이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float RecheckIntervalSeconds = 0.0f;

	// 안전 대응을 실행한 뒤 같은 대응을 다시 실행할 수 있을 때까지의 대기 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float CooldownSeconds = 0.0f;

	// 이동 목적지가 NavMesh 위에 존재해야만 유효한 경로로 인정할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	bool bRequireDestinationNavMesh = false;

	// 점프 또는 특수 이동 링크에 진입하기 전에 목적지를 미리 검사할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	bool bCheckBeforeLinkEntry = false;




};
