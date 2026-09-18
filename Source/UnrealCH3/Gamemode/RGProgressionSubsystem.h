// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RGEXPLevelRow.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RGUpgradeOption.h"
#include "RGProgressionSubsystem.generated.h"

class UDataTable;
class ARGBaseWeapon;
//레벨 업 시 UI가 받는 신호
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUpReady, const TArray<FRGUpgradeOption>&, Options);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUpgradeApplied, FName, UpgradeId, int32, NewStackCount);
//경험치 바 UI 갱신용 신호 (경험치 바 없다면 무시)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnExperienceChanged, float, CurrentExperience, float, RequiredExperience, int32, CurrentLevel);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreUpgradeReady, const TArray<FRGUpgradeOption>&, Options);

// [추가] 핵심 강화 1개가 실제 적용된 직후 외부 시스템에 알린다.
// GameMode / Portal이 강화 완료 시점을 직접 Progression 내부 구현에 결합하지 않게 한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnCoreUpgradeApplied,
	FName, UpgradeId,
	int32, ActiveCoreUpgradeCount
);

//레벨과 강화 스택을 들고 있을 저장소

UCLASS()
class UNREALCH3_API URGProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// ===== UI가 바인딩할 델리게이트 ==================================================
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnLevelUpReady OnLevelUpReady;

	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable)
	FOnUpgradeApplied OnUpgradeApplied;

	// ========== 레벨 및 일반 강화 함수 ================================================
	//게임모드에서 호출
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void InitializeExperienceCurve(UDataTable* InExperienceCurveTable);

	//강화 카탈로그 등록
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void InitializeGeneralUpgrades(UDataTable* InGeneralUpgradeTable);

	//적 AI에서 호출 . 적 Ai 사망 시 경험치 획득
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void GrantExperience(float Amount);


	//디버깅용 강제 레벨업 -> 콘솔 창에 등록해서 사용 예정
	UFUNCTION(BlueprintCallable, Category = "Progression|Debug")
	void DebugForceLevelUp();


	// UI의 "HandleUpgradeSelected(CardIndex)" 이벤트에서 그대로 호출하면 됨.
	// 카드 몇 번째를 클릭했는지 index로 적용
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ApplyUpgradeByIndex(int32 CardIndex);

	//UI가 강화 카드를 선택 했을 때 호출
	//추후 수정 예정
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ApplyUpgrade(FName UpgradeId);

	//특정 강화를 몇번 선택했는지 반환해줌. 무기나 UI등에서 호출
	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetUpgradeStackCount(FName UpgradeId) const;

	//스택 1개당 효과량 조회
	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetUpgradeEffectAmount(FName UpgradeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetCurrentExperience() const { return CurrentExperience; }

	// 사망 시 캐릭터에서 호출: 레벨/경험치/강화 스택을 전부 초기화
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ResetRun();

private:
	// 현재 레벨(1부터 시작)에서 다음 레벨까지 필요한 경험치를 데이터테이블에서 찾아옴.
	// 테이블에 더 높은 레벨 행이 없으면(최종레벨) nullptr 반환 -> 그 이상은 레벨업 안 함.
	const FRGEXPLevelRow* FindCurrentLevelRow() const;

	// 경험치가 충분하면 레벨업 1회 처리 (초과 경험치는 다음 레벨로 이월). 레벨업했으면 true.
	bool TryLevelUpOnce();

	// 대기 중인 레벨업이 있으면 그중 하나를 실제로 UI에 띄움 (동시에 여러 장 안 띄우고 순차적으로)
	void PresentNextPendingLevelUpIfAny();

	TArray<FRGUpgradeOption> GenerateUpgradeOptions(int32 Count) const;

protected:
	// 현재 플레이어가 들고 있는 무기를 가져옴 (없으면 nullptr)
	ARGBaseWeapon* GetCurrentEquippedWeapon() const;
	//====================================================================================

	// ================ 일반 강화 및 경험치 변수 ===========================================
	//강화 및 경험치 로직 데이터테이블
	UPROPERTY()
	UDataTable* ExperienceCurveTable = nullptr;
	UPROPERTY()
	UDataTable* GeneralUpgradeTable = nullptr;
	UPROPERTY()
	UDataTable* CoreUpgradeTable = nullptr;


	int32 CurrentLevel = 1;
	float CurrentExperience = 0.f;

	// 한 번에 여러 레벨을 올랐을 때(연속 레벨업), 아직 선택 안 한 레벨업이 몇 개 남았는지
	int32 PendingLevelUpCount = 0;

	// 지금 UI에 강화 선택 카드가 떠 있는 중인지 (떠 있으면 다음 대기열을 바로 안 띄움)
	bool bIsPresentingUpgradeChoice = false;

	// 강화 ID별 선택 횟수. 최종 수치가 아니라 "몇 번 골랐는지"만 저장해서,
	// 선택 순서와 무관하게 항상 같은 최종 결과가 나오게 함.
	TMap<FName, int32> UpgradeStacks;

	// 방금 UI에 띄운 후보 배열. ApplyUpgradeByIndex()가 CardIndex -> UpgradeId를 여기서 찾음.
	UPROPERTY()
	TArray<FRGUpgradeOption> LastPresentedOptions;

	//=================================================================================


public:
	// ======= 핵심 강화 함수 ==========================================================

	UFUNCTION(BlueprintCallable, Category = "Progression|CoreUpgrade")
	void InitializeCoreUpgrades(UDataTable* InCoreUpgradeTable);

	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade")
	bool HasCoreUpgrade(FName UpgradeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade")
	bool CanAcquireMoreCoreUpgrades() const;

	TArray<FRGUpgradeOption> GenerateCoreUpgradeOptions(int32 Count) const;

	// [기존 호환] 기본 3개 후보
	UFUNCTION(BlueprintCallable, Category = "Progression|CoreUpgrade")
	void PresentCoreUpgradeChoice();

	// [추가] DT_UpgradeGrantConfig의 CandidateCount를 그대로 사용할 수 있는 진입점
	UFUNCTION(BlueprintCallable, Category = "Progression|CoreUpgrade")
	bool PresentCoreUpgradeChoiceWithCount(int32 CandidateCount);


	// =========================================================
	// [추가] 맵 전환용 핵심 강화 예약 시스템
	// =========================================================
	//
	// 전투맵 StageClear 시에는 UI를 바로 띄우지 않고 Queue만 저장한다.
	// RGProgressionSubsystem은 GameInstanceSubsystem이므로
	// L_Map_TransitHUB로 OpenLevel 되어도 이 값이 유지된다.
	//
	// RestHub의 BP_CombatUIManager가 OnCoreUpgradeReady에 Bind한 뒤
	// ConsumePendingCoreUpgradeChoice()를 호출하면 그때 실제 UI 이벤트가 발생한다.
	// =========================================================

	/**
	 * 핵심 강화 지급을 "예약"만 한다.
	 * 여기서는 OnCoreUpgradeReady를 Broadcast하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|CoreUpgrade|Pending")
	bool QueueCoreUpgradeChoice(
		FName GrantId,
		int32 CandidateCount
	);


	/**
	 * 예약된 핵심 강화를 실제 후보 생성 + UI Broadcast로 소비한다.
	 *
	 * 호출 위치:
	 * BP_CombatUIManager BeginPlay
	 * -> Bind Event to On Core Upgrade Ready
	 * -> Consume Pending Core Upgrade Choice
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|CoreUpgrade|Pending")
	bool ConsumePendingCoreUpgradeChoice();


	/** 예약된 핵심 강화가 있는지 확인 */
	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade|Pending")
	bool HasPendingCoreUpgradeChoice() const
	{
		return bHasPendingCoreUpgradeChoice;
	}


	/** 현재 예약된 GrantId 확인 */
	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade|Pending")
	FName GetPendingCoreUpgradeGrantId() const
	{
		return PendingCoreUpgradeGrantId;
	}


	/** 현재 UI에 표시 중인 GrantId 확인 */
	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade|Pending")
	FName GetActiveCoreUpgradeGrantId() const
	{
		return ActiveCoreUpgradeGrantId;
	}


	// 카드 선택 시 호출
	UFUNCTION(BlueprintCallable, Category = "Progression|CoreUpgrade")
	void ApplyCoreUpgrade(FName UpgradeId);

	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade")
	int32 GetActiveCoreUpgradeCount() const
	{
		return ActiveCoreUpgrades.Num();
	}

	UFUNCTION(BlueprintPure, Category = "Progression|CoreUpgrade")
	bool IsPresentingCoreUpgradeChoice() const
	{
		return bIsPresentingCoreUpgradeChoice;
	}

	const FRGCoreUpgradeRow* FindCoreUpgradeRow(FName UpgradeId) const;

	UPROPERTY()
	TArray<FName> ActiveCoreUpgrades;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CoreUpgrade")
	int32 MaxCoreUpgradeCount = 2;



public:
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnCoreUpgradeReady OnCoreUpgradeReady;

	// [추가] 핵심 강화 적용 완료 신호
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnCoreUpgradeApplied OnCoreUpgradeApplied;

protected:
	UPROPERTY()
	TArray<FRGUpgradeOption> LastPresentedCoreOptions;

	UPROPERTY()
	bool bIsPresentingCoreUpgradeChoice = false;


	// =========================================================
	// [추가] 맵 전환을 견디는 핵심 강화 예약 상태
	// =========================================================

	/** 전투맵에서 예약된 핵심 강화가 존재하는지 */
	UPROPERTY()
	bool bHasPendingCoreUpgradeChoice = false;

	/** DT_UpgradeGrantConfig RowName. 예: Evolution01 */
	UPROPERTY()
	FName PendingCoreUpgradeGrantId = NAME_None;

	/** RestHub에서 표시할 후보 카드 수 */
	UPROPERTY()
	int32 PendingCoreUpgradeCandidateCount = 0;


	/**
	 * [추가] 전투맵에서 Queue할 당시 실제 장착 중이던 무기 클래스.
	 *
	 * OpenLevel로 TransitHUB에 들어가면 기존 Pawn/Weapon Actor는 파괴되기 때문에
	 * RestHub BeginPlay 시점의 GetCurrentEquippedWeapon()은 nullptr일 수 있다.
	 *
	 * 따라서 후보 필터링에 필요한 "어떤 무기를 들고 있었는지"를
	 * Actor 포인터가 아니라 Class로 GameInstanceSubsystem에 보존한다.
	 */
	UPROPERTY()
	TSubclassOf<ARGBaseWeapon> PendingCoreUpgradeWeaponClass;


	/**
	 * Consume 후 카드가 실제 선택되기 전까지 유지하는 GrantId.
	 * ApplyCoreUpgrade 성공 후 NAME_None으로 정리된다.
	 */
	UPROPERTY()
	FName ActiveCoreUpgradeGrantId = NAME_None;


public:
	// 추가: 디버깅용 강제 핵심강화 선택 트리거 -> 콘솔 창에 등록해서 사용 예정
	UFUNCTION(BlueprintCallable, Category = "Progression|Debug")
	void DebugForceCoreUpgradeChoice();

	// 추가: 디버깅용 이름으로 직접 핵심강화 적용 (예: "Ricochet")
	UFUNCTION(BlueprintCallable, Category = "Progression|Debug")
	void DebugApplyCoreUpgradeByName(const FString& UpgradeName);



	//===================================================================================
};