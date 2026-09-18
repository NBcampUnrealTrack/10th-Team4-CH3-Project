// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGGameModeBase.generated.h"

class UUserWidget;
class UDataTable;


// =========================================================
// Run State
// =========================================================

UENUM(BlueprintType)
enum class ERunState : uint8
{
	Init        UMETA(DisplayName = "Initialize"),
	Combat      UMETA(DisplayName = "Combat"),
	Pause       UMETA(DisplayName = "Pause"),
	Upgrade     UMETA(DisplayName = "Upgrade"),
	RestHub     UMETA(DisplayName = "Rest Hub"),
	Result      UMETA(DisplayName = "Result"),
	Loading     UMETA(DisplayName = "Loading")
};


// =========================================================
// Death Reason
// =========================================================

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
	Killed      UMETA(DisplayName = "Killed"),
	TimeOut     UMETA(DisplayName = "Time Out")
};


// =========================================================
// Input Mode
// =========================================================

UENUM(BlueprintType)
enum class ERGInputMode : uint8
{
	GameOnly        UMETA(DisplayName = "Game Only"),
	UIOnly          UMETA(DisplayName = "UI Only"),
	GameAndUI       UMETA(DisplayName = "Game And UI")
};


// =========================================================
// GameMode Event Delegates
// =========================================================

/**
 * Run State가 변경되었을 때 발생
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnRunStateChanged,
	ERunState, OldState,
	ERunState, NewState
);


/**
 * 남은 시간이 변경되었을 때 발생
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnRemainingTimeChanged,
	float, NewRemainingTime
);


/**
 * Kill Count가 변경되었을 때 발생
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnKillCountChanged,
	int32, CurrentKills,
	int32, TargetKills
);


/**
 * [추가] 점수가 변경되었을 때 발생
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnScoreChanged,
	int32, CurrentScore,
	int32, AddedScore
);


/**
 * Stage Clear가 발생했을 때
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FOnStageCleared
);


/**
 * Game Over가 발생했을 때
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnGameOver,
	EDeathReason, Reason
);


// =========================================================
// GameMode
// =========================================================

/**
 * Run & Gun 기본 GameMode
 *
 * 담당:
 * - Run State
 * - Run Timer
 * - Kill Count
 * - Stage Clear / Game Over
 * - 기본 UI
 * - 기본 Input Mode
 *
 * 다른 시스템과의 연결은 Delegate를 통해 느슨하게 유지한다.
 */
UCLASS()
class UNREALCH3_API ARGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()


	// =========================================================
	// Unreal 기본
	// =========================================================

public:

	ARGGameModeBase();


protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;


	// =========================================================
	// GameMode Events
	// =========================================================

public:

	/**
	 * Run State 변경 알림
	 *
	 * Blueprint에서 Bind / Assign 가능
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnRunStateChanged OnRunStateChanged;


	/**
	 * 남은 시간 변경 알림
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnRemainingTimeChanged OnRemainingTimeChanged;


	/**
	 * Kill Count 변경 알림
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnKillCountChanged OnKillCountChanged;


	/**
	 * [추가] 점수 변경 알림
	 *
	 * BP에서 점수 HUD를 연결할 때 사용한다.
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnScoreChanged OnScoreChanged;


	/**
	 * Stage Clear 알림
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnStageCleared OnStageCleared;


	/**
	 * Game Over 알림
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnGameOver OnGameOver;


	// =========================================================
	// GameMode Type
	// =========================================================

public:

	/**
	 * 이 GameMode에서 Run System을 사용할지 여부
	 *
	 * Combat GameMode:
	 * true
	 *
	 * MainMenu GameMode:
	 * false
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GameMode|Run"
	)
	bool bStartRunSystem;



	// =========================================================
	// Stage Config DataTable
	// =========================================================

public:

	/**
	 * [추가] 스테이지별 제한시간 / 목표 처치 수 등을 읽는 DataTable.
	 *
	 * Row Struct:
	 * FRGStageConfigRow
	 *
	 * BP_RGGameModeBase Class Defaults에서
	 * DT_StageConfig를 지정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	TObjectPtr<UDataTable> StageConfigTable;


	/**
	 * [추가] 현재 레벨 이름으로 StageConfig Row를 찾지 못했을 때
	 * 사용할 fallback RowName.
	 *
	 * 현재 Map01 Blockout 테스트에서도 바로 사용할 수 있도록
	 * 기본값은 Stage01로 둔다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	FName DefaultStageConfigRowName;


	/**
	 * [추가] 실제 적용된 DT_StageConfig의 RowName.
	 * 예: Stage01, Stage02
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	FName CurrentStageConfigRowName;


	/**
	 * [추가] 현재 적용된 스테이지의 StageId.
	 * StageId는 실제 Unreal Level Asset 이름과 맞추는 것을 권장.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	FName CurrentStageId;


	/**
	 * [추가] 현재 StageConfig에 정의된 요구 코어 수.
	 * 현재 단계에서는 저장만 하고,
	 * Stage Clear 판정에는 아직 KillCount만 사용한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	int32 RequiredCoresToClear;


	/**
	 * [추가] StageConfig에 정의된 완료 후 논리 목적지.
	 * 예: RestHub, BossArena, Result
	 *
	 * 다음 단계에서 PortalConfig와 연결할 때 사용한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	FName StageCompletionDestination;


	/**
	 * [추가] 해당 Stage에서 지급할 전용 재료 ID.
	 * 예: Muzzle1, Grip1
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Stage"
	)
	FName StageExclusiveMaterial;


	/**
	 * [추가] 현재 맵에 해당하는 StageConfig Row를 찾아
	 * GameMode의 제한시간/목표 처치 수 등에 적용한다.
	 *
	 * 현재 LevelName == StageId를 우선 검색하고,
	 * 실패하면 DefaultStageConfigRowName을 사용한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Config|Stage"
	)
	bool ApplyStageConfigForCurrentMap();


	UFUNCTION(
		BlueprintPure,
		Category = "Config|Stage"
	)
	FName GetCurrentStageConfigRowName() const
	{
		return CurrentStageConfigRowName;
	}


	UFUNCTION(
		BlueprintPure,
		Category = "Config|Stage"
	)
	FName GetStageCompletionDestination() const
	{
		return StageCompletionDestination;
	}


	UFUNCTION(
		BlueprintPure,
		Category = "Config|Stage"
	)
	int32 GetRequiredCoresToClear() const
	{
		return RequiredCoresToClear;
	}




	// =========================================================
	// Score Config DataTable
	// =========================================================

public:

	/**
	 * [추가] 사건별 점수를 읽는 DataTable.
	 *
	 * Row Struct:
	 * FRGScoreConfigRow
	 *
	 * BP_RGGameModeBase Class Defaults에서
	 * DT_ScoreConfig를 지정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Score"
	)
	TObjectPtr<UDataTable> ScoreConfigTable;


	/**
	 * [추가] 현재 스테이지 GameMode에서 누적된 점수.
	 *
	 * 현재 단계에서는 "현재 맵 점수"다.
	 * 맵 이동 뒤에도 총점을 유지하려면 이후 GameInstance/Subsystem으로
	 * 누적값을 옮기면 된다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Score"
	)
	int32 CurrentScore;


	/**
	 * [추가] DT_ScoreConfig의 RowName으로 점수를 적용한다.
	 *
	 * 예:
	 * AddScoreEvent("NormalEnemyKilled")
	 * AddScoreEvent("EliteEnemyKilled")
	 * AddScoreEvent("BossKilled")
	 *
	 * 반환값:
	 * 실제로 이번 호출에서 더해진 점수
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Score"
	)
	int32 AddScoreEvent(
		FName ScoreEventId
	);


	/**
	 * [추가] ScoreConfig Row의 기본 점수만 조회한다.
	 * Row를 찾지 못하면 0을 반환한다.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Run|Score"
	)
	int32 GetScoreValue(
		FName ScoreEventId
	) const;


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Score"
	)
	int32 GetCurrentScore() const
	{
		return CurrentScore;
	}



	// =========================================================
	// Run State
	// =========================================================

public:

	/**
	 * 현재 Run State
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|State"
	)
	ERunState CurrentState;


	/**
	 * Run State 변경
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|State"
	)
	void ChangeRunState(
		ERunState NewState
	);


	/**
	 * 현재 Run State Getter
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Run|State"
	)
	ERunState GetRunState() const
	{
		return CurrentState;
	}


	// =========================================================
	// Run Timer
	// =========================================================

protected:

	/**
	 * Run Timer Handle
	 */
	FTimerHandle RunTimerHandle;


	UFUNCTION()
	void UpdateRunTimer();


public:

	/**
	 * 현재 남은 시간
	 *
	 * DT_StageConfig가 지정되어 있으면 TimeLimitSeconds가 적용된다.
	 * DataTable을 찾지 못한 경우 Constructor 기본값을 fallback으로 사용한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadWrite,
		Category = "Run|Timer",
		meta = (ClampMin = "0.0")
	)
	float RemainingTime;


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Timer"
	)
	float GetRemainingTime() const
	{
		return RemainingTime;
	}


	// =========================================================
	// Kill / Stage Clear
	// =========================================================

public:

	/**
	 * 현재 처치 수
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Kill"
	)
	int32 CurrentKills;


	/**
	 * Stage Clear에 필요한 처치 수
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Run|Kill",
		meta = (ClampMin = "0")
	)
	int32 TargetKillsToClear;


	/**
	 * Enemy Death가 GameMode로 들어오는 진입점
	 *
	 * 기존 Blueprint 호환성을 위해 이름 유지
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Kill"
	)
	void OnEnemyDied();


	/**
	 * 현재 Stage Clear 조건 확인
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Run|Logic"
	)
	bool CheckStageClearCondition() const;


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Kill"
	)
	int32 GetCurrentKills() const
	{
		return CurrentKills;
	}


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Kill"
	)
	int32 GetTargetKillsToClear() const
	{
		return TargetKillsToClear;
	}


	// =========================================================
	// End Logic
	// =========================================================

public:

	/**
	 * Player Death / TimeOut / Stage Clear 조건 확인
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Logic"
	)
	void CheckEndCondition(
		bool bIsPlayerDead,
		bool bIsTimeOut
	);


protected:

	void ExecuteGameOver(
		EDeathReason Reason
	);


	void ExecuteStageClear();


	bool bIsRunEnded;

	bool bIsStageCleared;


	// =========================================================
	// System Validation
	// =========================================================

public:

	UFUNCTION(
		BlueprintCallable,
		Category = "System"
	)
	bool VerifySystems();


	// =========================================================
	// UI
	// =========================================================

public:

	/**
	 * BeginPlay 시 GameMode가 직접
	 * 기본 Widget을 생성할지 여부
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	bool bCreateDefaultUI;


	/**
	 * GameMode가 직접 생성할 기본 Widget
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	TSubclassOf<UUserWidget> DefaultUIClass;


	/**
	 * 현재 생성된 기본 Widget
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	TObjectPtr<UUserWidget> DefaultUIWidget;


	/**
	 * 기본 UI ZOrder
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	int32 DefaultUIZOrder;


	/**
	 * 기본 UI 생성
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "UI"
	)
	UUserWidget* CreateDefaultUI();


	/**
	 * 기본 UI 제거
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "UI"
	)
	void RemoveDefaultUI();


	/**
	 * 현재 생성된 기본 UI Getter
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "UI"
	)
	UUserWidget* GetDefaultUIWidget() const
	{
		return DefaultUIWidget;
	}


	// =========================================================
	// Input
	// =========================================================

public:

	/**
	 * BeginPlay에서 자동 Input Mode 적용 여부
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input|Default"
	)
	bool bApplyDefaultInputMode;


	/**
	 * GameOnly / UIOnly / GameAndUI
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input|Default"
	)
	ERGInputMode DefaultInputMode;


	/**
	 * 마우스 커서 표시 여부
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input|Default"
	)
	bool bShowMouseCursor;


	/**
	 * Input Mode 설정 적용
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Input"
	)
	void ApplyDefaultInputSettings();


	// =========================================================
	// Progression
	// =========================================================

public:

	/**
	 * 레벨별 필요 경험치 DataTable
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Progression"
	)
	TObjectPtr<UDataTable> ExperienceCurveTable;


	/**
	 * 일반 강화 DataTable
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Progression"
	)
	TObjectPtr<UDataTable> GeneralUpgradeTable;


	/**
	 * 핵심 강화 DataTable
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Progression"
	)
	TObjectPtr<UDataTable> CoreUpgradeTable;
};