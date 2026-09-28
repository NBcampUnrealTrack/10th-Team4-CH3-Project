// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
// [추가] DT_RunFlowConfig의 Enum/Row Struct를 GameMode에서도 직접 사용
#include "Gamemode/DataTableStruct/RGRunConfigRows.h"
#include "RGGameModeBase.generated.h"

class UUserWidget;
class UDataTable;
class ARGBaseWeapon;
class ARGCharacter;


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
 * [추가] DT_RunFlowConfig의 정책이 실제 GameMode에 적용되었을 때 발생
 *
 * AI / UI 시스템이 GameMode를 직접 참조하지 않고도
 * 현재 정책을 받을 수 있도록 Delegate로 노출한다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnRunFlowPolicyChanged,
	ERGRunInputPolicy, InputPolicy,
	ERGRunTimePolicy, TimePolicy,
	ERGRunAIState, AIState,
	FName, TopUI
);


/**
 * [추가] Objective 진행도가 바뀔 때 발생
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnObjectiveProgressChanged,
	FName, ObjectiveId,
	int32, CurrentValue,
	int32, RequiredValue,
	bool, bCompleted
);


/**
 * [추가] DT_UpgradeGrantConfig로 시작된 핵심 강화 선택이 끝났을 때 발생
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnUpgradeGrantCompleted,
	FName, GrantId
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

	/**
	 * Result 상태에서는 기존 World의 Unpause를 막는다.
	 *
	 * Result UI의 Retry / Start Over / Main Menu 버튼이
	 * SetGamePaused(false)를 호출하더라도 기존 World의 Timer가
	 * 다시 진행되기 전에 OpenLevel로 새 World로 넘어가게 하기 위한 안전장치다.
	 */
	virtual bool ClearPause() override;


protected:

	virtual void BeginPlay() override;

	// [추가] 숫자 1/2/3 키를 GameMode에서 직접 감지하기 위해 Tick 사용
	virtual void Tick(
		float DeltaSeconds
	) override;

	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;



	// =========================================================
	// [추가] GameMode Weapon Slot Switch
	// =========================================================

public:

	/**
	 * 현재 프로토타입에서 숫자키 무기 전환을 사용할지 여부.
	 *
	 * true:
	 * 1 = Rifle
	 * 2 = Shotgun
	 * 3 = Railgun
	 *
	 * PlayerController / Character 입력 코드를 수정하지 않고
	 * GameMode에서 직접 숫자키를 감지한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GameMode|WeaponSwitch"
	)
	bool bEnableNumberKeyWeaponSwitch;


	/**
	 * 숫자 1 슬롯.
	 * BP_RGGameModeBase Class Defaults에서 BP_RGAssaultRifle 지정.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GameMode|WeaponSwitch"
	)
	TSubclassOf<ARGBaseWeapon> RifleWeaponClass;


	/**
	 * 숫자 2 슬롯.
	 * BP_RGGameModeBase Class Defaults에서 BP_RGShotgun 지정.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GameMode|WeaponSwitch"
	)
	TSubclassOf<ARGBaseWeapon> ShotgunWeaponClass;


	/**
	 * 숫자 3 슬롯.
	 * BP_RGGameModeBase Class Defaults에서 BP_RGRailgun 지정.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GameMode|WeaponSwitch"
	)
	TSubclassOf<ARGBaseWeapon> RailgunWeaponClass;


	/**
	 * [추가] 슬롯 번호로 현재 플레이어의 무기를 교체한다.
	 *
	 * 기존 ARGCharacter::EquipWeapon()을 그대로 사용하기 때문에
	 * 기존 WeaponSocket / 애니메이션 / OnWeaponEquipped /
	 * UIManager / HUDController 재바인딩 흐름을 유지한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "GameMode|WeaponSwitch"
	)
	bool SwitchPlayerWeaponSlot(
		int32 SlotIndex
	);


protected:

	/**
	 * [추가] 현재 Player Character를 가져와
	 * 기존 EquipWeapon()을 호출한다.
	 */
	bool EquipPlayerWeaponClass(
		TSubclassOf<ARGBaseWeapon> NewWeaponClass
	);



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
	 * [추가] RunFlow 정책 변경 알림
	 *
	 * BP_CombatZone / UIManager 등에서 필요하면 이 Delegate만 Bind하면 된다.
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnRunFlowPolicyChanged OnRunFlowPolicyChanged;


	/**
	 * [추가] Objective 진행도 변경 알림
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnObjectiveProgressChanged OnObjectiveProgressChanged;


	/**
	 * [추가] UpgradeGrant 완료 알림
	 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Run|Event"
	)
	FOnUpgradeGrantCompleted OnUpgradeGrantCompleted;


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
	// Run Flow Config DataTable
	// =========================================================

public:

	/**
	 * [추가] Run State별 입력 / 시간 / AI / UI 정책 DataTable.
	 *
	 * Row Struct:
	 * FRGRunFlowConfigRow
	 *
	 * 기존 DT_RunFlowConfig를 그대로 지정하면 된다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|RunFlow"
	)
	TObjectPtr<UDataTable> RunFlowConfigTable;


	/**
	 * [추가] 현재 실제 적용 중인 입력 정책.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Policy"
	)
	ERGRunInputPolicy CurrentInputPolicy;


	/**
	 * [추가] 현재 실제 적용 중인 시간 정책.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Policy"
	)
	ERGRunTimePolicy CurrentTimePolicy;


	/**
	 * [추가] 현재 DataTable이 요구하는 AI 상태.
	 *
	 * GameMode가 AI를 직접 제어하지 않고
	 * OnRunFlowPolicyChanged를 통해 외부 시스템에 전달한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Policy"
	)
	ERGRunAIState CurrentAIState;


	/**
	 * [추가] 현재 화면 최상단 UI의 논리 ID.
	 *
	 * 예: Inventory, UpgradeSelection, Result, Transition
	 * 실제 Widget 생성은 기존 UI 시스템이 담당한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Policy"
	)
	FName CurrentTopUI;


	/**
	 * [추가] 기획 문서/디버그용 전환 설명.
	 *
	 * DT의 AllowedTransition은 문자열이므로
	 * 런타임 상태 전환 검증에는 사용하지 않는다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Policy"
	)
	FString CurrentAllowedTransition;


	/**
	 * [추가] ERunState에 대응하는 DT_RunFlowConfig Row를 읽어
	 * 입력/시간 정책을 즉시 적용한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Config|RunFlow"
	)
	bool ApplyRunFlowConfigForState(
		ERunState State
	);


	/**
	 * [추가] 현재 ERunState가 어떤 DataTable Row를 사용하는지 반환.
	 *
	 * Init    -> WeaponSelect
	 * Combat  -> Combat
	 * Pause   -> InventoryPause
	 * Upgrade -> CoreUpgrade
	 * RestHub -> RestHub
	 * Loading -> Loading
	 * Result  -> Result
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Config|RunFlow"
	)
	FName GetRunFlowRowNameForState(
		ERunState State
	) const;


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Policy"
	)
	ERGRunAIState GetCurrentAIState() const
	{
		return CurrentAIState;
	}


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Policy"
	)
	FName GetCurrentTopUI() const
	{
		return CurrentTopUI;
	}


protected:

	/**
	 * [추가] DT_RunFlowConfig가 없거나 Row를 찾지 못했을 때
	 * 기존 GameMode 동작을 유지하기 위한 fallback.
	 */
	void ApplyLegacyRunFlowFallback(
		ERunState State
	);


	/**
	 * [추가] 입력 정책을 PlayerController에 실제 적용.
	 */
	void ApplyInputPolicy(
		ERGRunInputPolicy InputPolicy
	);


	/**
	 * [추가] 시간 정책을 RunTimerHandle에 실제 적용.
	 */
	void ApplyTimePolicy(
		ERGRunTimePolicy TimePolicy
	);




	// =========================================================
	// Objective Config DataTable
	// =========================================================

public:

	/**
	 * [추가] 목표 종류/필요값/완료 기여 설정.
	 * 기존 DT_ObjectiveConfig를 그대로 지정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Objective"
	)
	TObjectPtr<UDataTable> ObjectiveConfigTable;


	/**
	 * [안전 설정]
	 * 현재 프로젝트의 DataCore 실제 파괴 이벤트가 연결된 뒤 true로 켠다.
	 *
	 * false여도 DT_ObjectiveConfig와 진행값 API는 동작하지만
	 * Stage Clear를 막지는 않는다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Objective"
	)
	bool bEnforceCoreObjective;


	/**
	 * [안전 설정]
	 * AliveEnemyGate를 실제 StageClear 필수 조건으로 사용할지 여부.
	 *
	 * 현재 요구사항은 "목표 킬 수 달성 시 포탈 활성화"이므로
	 * 기본값은 false다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Objective"
	)
	bool bEnforceRemainingEnemyObjective;


	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Objective"
	)
	int32 CurrentCoresDestroyed;


	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Objective"
	)
	int32 CurrentRemainingEnemies;


	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Objective"
	)
	bool bBossDefeated;


	/**
	 * [추가] DataCore BP/Actor가 파괴되었을 때 호출하는 진입점.
	 * BP_TestDataCore 등 기존 Blueprint에서 호출 가능.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Objective"
	)
	void OnDataCoreDestroyed();


	/**
	 * [추가] SpawnManager -> GameFlowBridge가 현재 생존 적 수를 전달한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Objective"
	)
	void SetRemainingEnemyCount(
		int32 NewRemainingEnemyCount
	);


	/**
	 * [추가] Boss 사망 전용 진입점.
	 * GameFlowBridge가 ABossEnemy를 감지하면 이 함수를 사용한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Objective"
	)
	void OnBossKilled();


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Objective"
	)
	int32 GetCurrentCoresDestroyed() const
	{
		return CurrentCoresDestroyed;
	}


	UFUNCTION(
		BlueprintPure,
		Category = "Run|Objective"
	)
	int32 GetCurrentRemainingEnemies() const
	{
		return CurrentRemainingEnemies;
	}


protected:

	const FRGObjectiveConfigRow* FindObjectiveConfigRow(
		FName ObjectiveId
	) const;

	int32 ResolveObjectiveRequiredValue(
		FName ObjectiveId,
		const FRGObjectiveConfigRow& Row
	) const;

	void BroadcastObjectiveProgress(
		FName ObjectiveId
	);


	// =========================================================
	// Upgrade Grant Config DataTable
	// =========================================================

public:

	/**
	 * [추가] 스테이지별 핵심 강화 지급 타이밍/후보 수 설정.
	 * 기존 DT_UpgradeGrantConfig를 그대로 지정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|UpgradeGrant"
	)
	TObjectPtr<UDataTable> UpgradeGrantConfigTable;


	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|UpgradeGrant"
	)
	FName ActiveUpgradeGrantId;


	/**
	 * [추가] 현재 스테이지에 해당하는 Grant Row를 찾아 핵심 강화 UI를 시작.
	 * TriggerCondition 문자열 안에 현재 Stage RowName이 포함되는 방식을 사용한다.
	 *
	 * 현재 데이터:
	 * Evolution01 -> "Stage02 완료→RestHub 진입"
	 * Evolution02 -> "Stage04_PreBoss 완료"
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|UpgradeGrant"
	)
	bool TryStartConfiguredUpgradeGrant();


	/**
	 * [추가] 현재 Stage의 DT_UpgradeGrantConfig를 확인하고,
	 * UI를 즉시 띄우지 않고 RGProgressionSubsystem에 예약만 저장한다.
	 *
	 * Stage Clear -> Queue
	 * Portal -> L_Map_TransitHUB
	 * BP_CombatUIManager BeginPlay -> Consume
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|UpgradeGrant"
	)
	bool QueueConfiguredUpgradeGrant();


protected:

	UFUNCTION()
	void HandleCoreUpgradeApplied(
		FName UpgradeId,
		int32 ActiveCoreUpgradeCount
	);

	const FRGUpgradeGrantConfigRow* FindUpgradeGrantForCurrentStage(
		FName& OutGrantId
	) const;

	int32 PendingUpgradeSelections;



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

	void BroadcastInitialRunStatus();


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

	//UI 연동용 플레이 시간
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Timer")
	float InitialTimeLimit = 60.0f;

	UFUNCTION(BlueprintPure, Category = "Run|Timer")
	float GetElapsedRunTimeSeconds() const { return FMath::Max(0.0f, InitialTimeLimit - RemainingTime); }


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


	/**
	 * Result 진입 시 현재 World에 남아 있는 전투용 동적 Actor를 정리한다.
	 *
	 * 삭제 대상:
	 * - Enemy / Boss (ABaseEnemy 계열)
	 * - Projectile
	 * - AreaAttack
	 * - Grenade
	 *
	 * SpawnManager는 삭제하지 않고 추가 Spawn만 중지한다.
	 * UIManager / HUD / PlayerController / GameMode까지 지우면
	 * Result 버튼이 작동하지 않으므로 시스템 Actor는 유지한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Cleanup"
	)
	void CleanupTransientGameplayActors();


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