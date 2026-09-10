// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGGameModeBase.generated.h"

class UUserWidget;


// =========================================================
// 게임 진행 상태
// =========================================================

UENUM(BlueprintType)
enum class ERunState : uint8
{
	Init        UMETA(DisplayName = "초기화"),
	Combat      UMETA(DisplayName = "전투 중"),
	Pause       UMETA(DisplayName = "일시정지"),
	Upgrade     UMETA(DisplayName = "강화 중"),
	RestHub     UMETA(DisplayName = "휴식처"),
	Result      UMETA(DisplayName = "결과 화면"),
	Loading     UMETA(DisplayName = "로딩 중")
};


// =========================================================
// 게임 종료 이유
// =========================================================

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
	Killed      UMETA(DisplayName = "사망"),
	TimeOut     UMETA(DisplayName = "시간 초과")
};


// =========================================================
// 입력 모드
// =========================================================

UENUM(BlueprintType)
enum class ERGInputMode : uint8
{
	GameOnly        UMETA(DisplayName = "Game Only"),
	UIOnly          UMETA(DisplayName = "UI Only"),
	GameAndUI       UMETA(DisplayName = "Game And UI")
};


// =========================================================
// GameMode
// =========================================================

/**
 * Run & Gun 공통 GameMode
 *
 * 담당 기능
 *
 * 1. 게임 상태 관리
 * 2. 제한 시간 관리
 * 3. Kill Count 관리
 * 4. Stage Clear / Game Over
 * 5. 기본 UI 생성
 * 6. 기본 Input Mode 설정
 *
 * 실제 Pawn / Controller / HUD Class는
 * 이 클래스를 상속한 Blueprint GameMode에서 설정한다.
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
	// GameMode Type 설정
	// =========================================================

public:

	/**
	 * 이 GameMode에서 게임 진행 시스템을 시작할지 여부.
	 *
	 * Combat GameMode:
	 * True
	 *
	 * MainMenu GameMode:
	 * False
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GameMode|Run"
	)
	bool bStartRunSystem;


	// =========================================================
	// Run State
	// =========================================================

public:

	/**
	 * 현재 게임 상태
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|State"
	)
	ERunState CurrentState;


	/**
	 * 게임 상태 변경
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|State"
	)
	void ChangeRunState(
		ERunState NewState
	);


	/**
	 * 현재 상태 Getter
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
	 * 게임 시간 Timer Handle
	 */
	FTimerHandle RunTimerHandle;


	UFUNCTION()
	void UpdateRunTimer();


public:

	/**
	 * 게임 제한 시간
	 *
	 * BP_RGGameMode Class Defaults에서 조정 가능
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
		meta = (ClampMin = "1")
	)
	int32 TargetKillsToClear;


	UFUNCTION(
		BlueprintCallable,
		Category = "Run|Kill"
	)
	void OnEnemyDied();


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
	// 게임 종료
	// =========================================================

public:

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
	 * Widget을 생성할지 여부
	 *
	 * MainMenu:
	 * True
	 *
	 * Combat HUD Manager 사용:
	 * False
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	bool bCreateDefaultUI;


	/**
	 * GameMode가 직접 생성할 Widget
	 *
	 * 예:
	 * WBP_MainMenu
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	TSubclassOf<UUserWidget> DefaultUIClass;


	/**
	 * 실제 생성된 Widget
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	TObjectPtr<UUserWidget> DefaultUIWidget;


	/**
	 * UI ZOrder
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
	 * 현재 생성된 기본 UI
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
	 * 마우스 커서 표시
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input|Default"
	)
	bool bShowMouseCursor;


	/**
	 * Input Mode 실제 적용
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Input"
	)
	void ApplyDefaultInputSettings();
};