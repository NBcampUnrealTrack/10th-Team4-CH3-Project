#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RGRunConfigRows.generated.h"

// ============================================================
// 런/스테이지 설정 DataTable 공용 타입
//
// 이 파일은 아래 6개 DataTable의 Row Struct를 한 곳에 모아 둔다.
// - DT_StageConfig
// - DT_RunFlowConfig
// - DT_ObjectiveConfig
// - DT_PortalConfig
// - DT_UpgradeGrantConfig
// - DT_ScoreConfig
//
// 각 DataTable의 RowName(Name 컬럼)을 실제 ID로 사용한다.
// 따라서 Stage01, Combat, Kill_Default 같은 값은 별도 ID 필드로
// 중복 저장하지 않고 DataTable RowName으로 조회한다.
// ============================================================

// -------------------------
// DT_RunFlowConfig용 Enum
// -------------------------
UENUM(BlueprintType)
enum class ERGRunInputPolicy : uint8
{
	UIOnly   UMETA(DisplayName = "UI 전용"),
	Gameplay UMETA(DisplayName = "게임플레이"),
	Blocked  UMETA(DisplayName = "차단")
};

UENUM(BlueprintType)
enum class ERGRunTimePolicy : uint8
{
	Paused      UMETA(DisplayName = "정지"),
	Countdown   UMETA(DisplayName = "감소"),
	NoObjective UMETA(DisplayName = "맵 목표 없음")
};

UENUM(BlueprintType)
enum class ERGRunAIState : uint8
{
	Paused   UMETA(DisplayName = "정지"),
	Active   UMETA(DisplayName = "활성"),
	Disabled UMETA(DisplayName = "비활성")
};

// -------------------------
// DT_ObjectiveConfig용 Enum
// -------------------------
UENUM(BlueprintType)
enum class ERGObjectiveType : uint8
{
	KillCount         UMETA(DisplayName = "Kill Count"),
	DataCoreDestroyed UMETA(DisplayName = "Data Core Destroyed"),
	RemainingEnemy    UMETA(DisplayName = "Remaining Enemy"),
	BossKilled        UMETA(DisplayName = "Boss Killed")
};

// -------------------------
// DT_PortalConfig용 Enum
// -------------------------
UENUM(BlueprintType)
enum class ERGPortalInitialState : uint8
{
	Hidden        UMETA(DisplayName = "Hidden"),
	VisibleLocked UMETA(DisplayName = "Visible Locked")
};

UENUM(BlueprintType)
enum class ERGPortalInteractionType : uint8
{
	EnterOrUse UMETA(DisplayName = "진입 또는 E"),
	EnterOnly  UMETA(DisplayName = "진입")
};

// ============================================================
// DT_StageConfig
// 맵별 목표/제한시간/완료 후 이동 설정
// RowName 예: Stage01, Stage02, Stage04_Boss
// ============================================================
USTRUCT(BlueprintType)
struct UNREALCH3_API FRGStageConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	// 실제 스테이지/맵 식별자. 예: Map_01, BossArena
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FName StageId = NAME_None;

	// 전투 제한 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0.0", Units = "s"))
	float TimeLimitSeconds = 150.0f;

	// 스테이지 완료에 필요한 처치 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0"))
	int32 RequiredKills = 0;

	// 스테이지 완료에 필요한 데이터 코어 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0"))
	int32 RequiredCores = 0;

	// 완료 후 이동 목적지 논리 ID. 예: RestHub, BossArena, Result
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	FName CompletionDestination = NAME_None;

	// 스테이지 전용 지급/해금 재료 ID. 없으면 None
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FName ExclusiveMaterial = NAME_None;

	// 어떤 시스템이 이 설정을 사용하는지 문서화용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Documentation")
	FString AffectedParts;
};

// ============================================================
// DT_RunFlowConfig
// 런 상태별 입력/시간/AI/UI/전환 정책
// RowName 예: Combat, RestHub, Loading
// ============================================================
USTRUCT(BlueprintType)
struct UNREALCH3_API FRGRunFlowConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Flow")
	ERGRunInputPolicy InputPolicy = ERGRunInputPolicy::Gameplay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Flow")
	ERGRunTimePolicy TimePolicy = ERGRunTimePolicy::Paused;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Flow")
	ERGRunAIState AIState = ERGRunAIState::Paused;

	// 화면 최상단 UI 논리 ID. 없으면 None
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FName TopUI = NAME_None;

	// 허용되는 다음 상태/조건. 복수 조건이 있어 문자열로 유지한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	FString AllowedTransition;

	// 어떤 시스템이 이 설정을 사용하는지 문서화용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Documentation")
	FString AffectedParts;
};

// ============================================================
// DT_ObjectiveConfig
// 목표 종류/필요값/표시/완료 기여 설정
// RowName 예: Kill_Default, Core_Default
// ============================================================
USTRUCT(BlueprintType)
struct UNREALCH3_API FRGObjectiveConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	ERGObjectiveType ObjectiveType = ERGObjectiveType::KillCount;

	// 고정 필요값. bUseStageConfigValue=true이면 StageConfig 값을 사용하고 이 값은 보조/기본값으로만 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0"))
	int32 RequiredValue = 0;

	// Kill/Core처럼 스테이지별 필요값을 DT_StageConfig에서 읽을지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool bUseStageConfigValue = false;

	// 같은 사건을 중복으로 카운트할 수 있는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool bAllowDuplicateEvents = false;

	// HUD/월드 UI 표시 방식 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FString DisplayMode;

	// 스테이지 완료 판정에서의 역할
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString CompletionContribution;
};

// ============================================================
// DT_PortalConfig
// 포탈 초기 상태/활성 조건/상호작용/목적지 설정
// RowName 예: CombatExit, RestHubExit, BossEntrance
// ============================================================
USTRUCT(BlueprintType)
struct UNREALCH3_API FRGPortalConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	ERGPortalInitialState InitialState = ERGPortalInitialState::Hidden;

	// 포탈 표시/잠금 해제 조건의 논리 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FName VisibilityCondition = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	ERGPortalInteractionType InteractionType = ERGPortalInteractionType::EnterOrUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal", meta = (ClampMin = "0.0", Units = "s"))
	float TransitionDelaySeconds = 0.8f;

	// RestHub, NextStageId, BossArena 등의 논리 목적지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FName Destination = NAME_None;
};

// ============================================================
// DT_UpgradeGrantConfig
// 핵심 강화 지급 타이밍/후보 수/시간 정지/완료 후 흐름
// RowName 예: Evolution01, Evolution02
// ============================================================
USTRUCT(BlueprintType)
struct UNREALCH3_API FRGUpgradeGrantConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	// 강화 지급 발생 조건 설명/논리 키
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Grant")
	FString TriggerCondition;

	// 화면에 구성할 후보 수. 예: 3개 중 1개이면 3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Grant", meta = (ClampMin = "1"))
	int32 CandidateCount = 3;

	// 실제 선택 수. 현재 기획은 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Grant", meta = (ClampMin = "1"))
	int32 SelectionCount = 1;

	// 후보 구성 규칙 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Grant")
	FString CandidateComposition;

	// 강화 선택 중 시간 정지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Grant")
	bool bPauseTime = true;

	// 강화 선택 완료 후 흐름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Grant")
	FString CompletionResult;
};

// ============================================================
// DT_ScoreConfig
// 사건별 점수와 기록 반영 규칙
// RowName 예: NormalEnemyKilled, BossKilled
// ============================================================
USTRUCT(BlueprintType)
struct UNREALCH3_API FRGScoreConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 BaseScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	bool bAllowDuplicateEvents = false;

	// 처치·총점, 목표·총점, 최종 총점, 반영 안 함 등 기록 규칙
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	FString RecordContribution;
};
