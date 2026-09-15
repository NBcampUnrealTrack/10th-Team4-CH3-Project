// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGGameModeBase.generated.h"

class UUserWidget;
class UDataTable;


// =========================================================
// ���� ���� ����
// =========================================================

UENUM(BlueprintType)
enum class ERunState : uint8
{
	Init        UMETA(DisplayName = "�ʱ�ȭ"),
	Combat      UMETA(DisplayName = "���� ��"),
	Pause       UMETA(DisplayName = "�Ͻ�����"),
	Upgrade     UMETA(DisplayName = "��ȭ ��"),
	RestHub     UMETA(DisplayName = "�޽�ó"),
	Result      UMETA(DisplayName = "��� ȭ��"),
	Loading     UMETA(DisplayName = "�ε� ��")
};


// =========================================================
// ���� ���� ����
// =========================================================

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
	Killed      UMETA(DisplayName = "���"),
	TimeOut     UMETA(DisplayName = "�ð� �ʰ�")
};


// =========================================================
// �Է� ���
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
 * Run & Gun ���� GameMode
 *
 * ��� ���
 *
 * 1. ���� ���� ����
 * 2. ���� �ð� ����
 * 3. Kill Count ����
 * 4. Stage Clear / Game Over
 * 5. �⺻ UI ����
 * 6. �⺻ Input Mode ����
 *
 * ���� Pawn / Controller / HUD Class��
 * �� Ŭ������ ����� Blueprint GameMode���� �����Ѵ�.
 */
UCLASS()
class UNREALCH3_API ARGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()


	// =========================================================
	// Unreal �⺻
	// =========================================================

public:

	ARGGameModeBase();


protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;


	// =========================================================
	// GameMode Type ����
	// =========================================================

public:

	/**
	 * �� GameMode���� ���� ���� �ý����� �������� ����.
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
	// Progression (경험치/레벨/강화)
	// =========================================================

public:
	// DT_ExperienceCurve 테이블 - > 언리얼 에디터 BP_RGGameMode Class Defaults 에서 지정
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Progression")
	UDataTable* ExperienceCurveTable = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
	UDataTable* GeneralUpgradeTable = nullptr;


	// =========================================================
	// Run State
	// =========================================================

public:

	/**
	 * ���� ���� ����
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|State"
	)
	ERunState CurrentState;


	/**
	 * ���� ���� ����
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Run|State"
	)
	void ChangeRunState(
		ERunState NewState
	);


	/**
	 * ���� ���� Getter
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
	 * ���� �ð� Timer Handle
	 */
	FTimerHandle RunTimerHandle;


	UFUNCTION()
	void UpdateRunTimer();


public:

	/**
	 * ���� ���� �ð�
	 *
	 * BP_RGGameMode Class Defaults���� ���� ����
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
	 * ���� óġ ��
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Run|Kill"
	)
	int32 CurrentKills;


	/**
	 * Stage Clear�� �ʿ��� óġ ��
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
	// ���� ����
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
	 * BeginPlay �� GameMode�� ����
	 * Widget�� �������� ����
	 *
	 * MainMenu:
	 * True
	 *
	 * Combat HUD Manager ���:
	 * False
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	bool bCreateDefaultUI;


	/**
	 * GameMode�� ���� ������ Widget
	 *
	 * ��:
	 * WBP_MainMenu
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "UI|Default"
	)
	TSubclassOf<UUserWidget> DefaultUIClass;


	/**
	 * ���� ������ Widget
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
	 * �⺻ UI ����
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "UI"
	)
	UUserWidget* CreateDefaultUI();


	/**
	 * �⺻ UI ����
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "UI"
	)
	void RemoveDefaultUI();


	/**
	 * ���� ������ �⺻ UI
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
	 * BeginPlay���� �ڵ� Input Mode ���� ����
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
	 * ���콺 Ŀ�� ǥ��
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input|Default"
	)
	bool bShowMouseCursor;


	/**
	 * Input Mode ���� ����
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Input"
	)
	void ApplyDefaultInputSettings();
};