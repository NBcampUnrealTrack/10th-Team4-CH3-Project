#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGGameModeBase.generated.h"

class UUserWidget;
class ARGCharacter;
class ABaseEnemy;


// =========================================================
// Run State
// =========================================================

UENUM(BlueprintType)
enum class ERunState : uint8
{
	Init,
	Combat,
	Pause,
	Upgrade,
	RestHub,
	Result,
	Loading
};


// =========================================================
// Death Reason
// =========================================================

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
	Killed,
	TimeOut
};


// =========================================================
// Input Mode
// =========================================================

UENUM(BlueprintType)
enum class ERGInputMode : uint8
{
	GameOnly,
	UIOnly,
	GameAndUI
};


// =========================================================
// GameMode
// =========================================================

UCLASS()
class UNREALCH3_API ARGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	ARGGameModeBase();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	// =========================================================
	// Run
	// =========================================================

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	bool bStartRunSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run")
	bool bSystemsReady;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run")
	bool bCombatStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run")
	bool bIsRunEnded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run")
	bool bIsStageCleared;

	UFUNCTION(BlueprintCallable, Category = "Run")
	void InitializeRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	bool StartCombat();

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsCombatActive() const;


	// =========================================================
	// State
	// =========================================================

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|State")
	ERunState CurrentState;

	UFUNCTION(BlueprintCallable, Category = "Run|State")
	void ChangeRunState(ERunState NewState);

	UFUNCTION(BlueprintPure, Category = "Run|State")
	ERunState GetRunState() const
	{
		return CurrentState;
	}


	// =========================================================
	// Timer
	// =========================================================

protected:

	FTimerHandle RunTimerHandle;

	UFUNCTION()
	void UpdateRunTimer();

public:

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Run|Timer",
		meta = (ClampMin = "1.0")
	)
	float RunTimeLimit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Timer")
	float RemainingTime;

	UFUNCTION(BlueprintPure, Category = "Run|Timer")
	float GetRemainingTime() const
	{
		return RemainingTime;
	}


	// =========================================================
	// Objective
	// =========================================================

public:

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Run|Objective",
		meta = (ClampMin = "0")
	)
	int32 TargetKillsToClear;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Run|Objective",
		meta = (ClampMin = "0")
	)
	int32 TargetCoresToClear;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Objective")
	int32 CurrentKills;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Objective")
	int32 CurrentCores;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Objective")
	int32 AliveEnemyCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Score")
	int32 CurrentScore;

	UFUNCTION(BlueprintCallable, Category = "Run|Objective")
	void RegisterEnemySpawned(ABaseEnemy* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Run|Objective")
	void UnregisterEnemy(ABaseEnemy* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Run|Objective")
	void RegisterEnemyKilled(ABaseEnemy* Enemy, int32 ScoreValue);

	UFUNCTION(BlueprintCallable, Category = "Run|Objective")
	void RegisterCoreDestroyed();

	UFUNCTION(BlueprintPure, Category = "Run|Objective")
	bool CheckStageClearCondition() const;

	UFUNCTION(BlueprintCallable, Category = "Run|Objective")
	void CheckObjectiveCompletion();

	UFUNCTION(BlueprintPure, Category = "Run|Objective")
	int32 GetCurrentKills() const
	{
		return CurrentKills;
	}

	UFUNCTION(BlueprintPure, Category = "Run|Objective")
	int32 GetCurrentCores() const
	{
		return CurrentCores;
	}

	UFUNCTION(BlueprintPure, Category = "Run|Objective")
	int32 GetAliveEnemyCount() const
	{
		return AliveEnemyCount;
	}

	UFUNCTION(BlueprintPure, Category = "Run|Score")
	int32 GetCurrentScore() const
	{
		return CurrentScore;
	}


	// =========================================================
	// Fail / Clear
	// =========================================================

public:

	UFUNCTION(BlueprintCallable, Category = "Run|Result")
	void NotifyPlayerDeath();

protected:

	void ExecuteGameOver(EDeathReason Reason);
	void ExecuteStageClear();


	// =========================================================
	// Systems
	// =========================================================

public:

	UFUNCTION(BlueprintCallable, Category = "System")
	bool VerifySystems();

	UFUNCTION(BlueprintCallable, Category = "System")
	void SetPlayerActionsAllowed(bool bAllowed);


	// =========================================================
	// UI
	// =========================================================

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Default")
	bool bCreateDefaultUI;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Default")
	TSubclassOf<UUserWidget> DefaultUIClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Default")
	TObjectPtr<UUserWidget> DefaultUIWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Default")
	int32 DefaultUIZOrder;

	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* CreateDefaultUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RemoveDefaultUI();


	// =========================================================
	// Input
	// =========================================================

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Default")
	bool bApplyDefaultInputMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Default")
	ERGInputMode DefaultInputMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Default")
	bool bShowMouseCursor;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void ApplyDefaultInputSettings();


private:

	// 중복 Spawn / Death 이벤트 방지
	UPROPERTY()
	TSet<TObjectPtr<ABaseEnemy>> AliveEnemies;
};