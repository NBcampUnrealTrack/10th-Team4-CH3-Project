#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGGameModeBase.generated.h"

class UUserWidget;

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

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
	Killed      UMETA(DisplayName = "사망"),
	TimeOut     UMETA(DisplayName = "시간 초과")
};

UENUM(BlueprintType)
enum class ERGInputMode : uint8
{
	GameOnly        UMETA(DisplayName = "Game Only"),
	UIOnly          UMETA(DisplayName = "UI Only"),
	GameAndUI       UMETA(DisplayName = "Game And UI")
};

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
	// Run System
	// =========================================================

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Run")
	bool bStartRunSystem;

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
	// Run Start / Ready
	// =========================================================

public:
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRunReady() const
	{
		return bIsRunReady;
	}

	UFUNCTION(BlueprintPure, Category = "Run")
	bool HasRunStarted() const
	{
		return bHasRunStarted;
	}

protected:
	void ResetRunData();
	void SetGameplayInputEnabled(bool bEnabled);

	bool bHasRunStarted;
	bool bIsRunReady;

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
		BlueprintReadWrite,
		Category = "Run|Timer",
		meta = (ClampMin = "0.0")
	)
	float RunDuration;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Timer"
	)
	float RemainingTime;

	UFUNCTION(BlueprintPure, Category = "Run|Timer")
	float GetRemainingTime() const
	{
		return RemainingTime;
	}

	// =========================================================
	// Kill / Score
	// =========================================================

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Kill")
	int32 CurrentKills;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Run|Kill",
		meta = (ClampMin = "1")
	)
	int32 TargetKillsToClear;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Score")
	int32 CurrentScore;

	UFUNCTION(BlueprintCallable, Category = "Run|Kill")
	void OnEnemyDied();

	UFUNCTION(BlueprintPure, Category = "Run|Logic")
	bool CheckStageClearCondition() const;

	UFUNCTION(BlueprintPure, Category = "Run|Kill")
	int32 GetCurrentKills() const
	{
		return CurrentKills;
	}

	UFUNCTION(BlueprintPure, Category = "Run|Kill")
	int32 GetTargetKillsToClear() const
	{
		return TargetKillsToClear;
	}

	UFUNCTION(BlueprintPure, Category = "Run|Score")
	int32 GetCurrentScore() const
	{
		return CurrentScore;
	}

	// =========================================================
	// End / Stage Clear / Victory
	// =========================================================

public:
	UFUNCTION(BlueprintCallable, Category = "Run|Logic")
	void CheckEndCondition(bool bIsPlayerDead, bool bIsTimeOut);

	// 포탈 진입 시 최종 승리 처리
	UFUNCTION(BlueprintCallable, Category = "Run|Logic")
	void ExecuteVictory();

	// 10킬 등 스테이지 클리어 발생 시
	// BP_RGGameModeBase에서 포탈 활성화 등에 사용
	UFUNCTION(BlueprintImplementableEvent, Category = "Run|Logic")
	void OnStageClear();

protected:
	void ExecuteGameOver(EDeathReason Reason);
	void ExecuteStageClear();

	bool bIsRunEnded;
	bool bIsStageCleared;

	// =========================================================
	// Validation
	// =========================================================

public:
	UFUNCTION(BlueprintCallable, Category = "System")
	bool VerifySystems();

	// =========================================================
	// UI
	// =========================================================

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Default")
	bool bCreateDefaultUI;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Default")
	TSubclassOf<UUserWidget> DefaultUIClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Default")
	TObjectPtr<UUserWidget> DefaultUIWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Default")
	int32 DefaultUIZOrder;

	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* CreateDefaultUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RemoveDefaultUI();

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetDefaultUIWidget() const
	{
		return DefaultUIWidget;
	}

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
};