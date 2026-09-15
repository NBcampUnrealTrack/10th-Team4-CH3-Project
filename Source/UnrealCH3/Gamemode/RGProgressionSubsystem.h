// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RGEXPLevelRow.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RGProgressionSubsystem.generated.h"

class UDataTable;
//레벨 업 시 UI가 받는 신호
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelUpReady, int32, NewLevel, int32, ChoiceCount);
//경험치 바 UI 갱신용 신호 (경험치 바 없다면 무시)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnExperienceChanged, float, CurrentExperience, float, RequiredExperience, int32, CurrentLevel);

//레벨과 강화 스택을 들고 있을 저장소

UCLASS()
class UNREALCH3_API URGProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//게임모드에서 호출
	UFUNCTION(BlueprintCallable , Category = "Progression")
	void InitializeExperienceCurve(UDataTable* InExperienceCurveTable);

	//적 AI에서 호출 . 적 Ai 사망 시 경험치 획득
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void GrantExperience(float Amount);

	//UI가 강화 카드를 선택 했을 때 호출
	//추후 수정 예정
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ApplyUpgrade(FName UpgradeId);

	//특정 강화를 몇번 선택했는지 반환해줌. 무기나 UI등에서 호출
	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetUpgradeStackCount(FName UpgradeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetCurrentExperience() const { return CurrentExperience; }

	// 사망 시 캐릭터에서 호출: 레벨/경험치/강화 스택을 전부 초기화
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ResetRun();

	// ===== UI가 바인딩할 델리게이트 =====
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnLevelUpReady OnLevelUpReady;

	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnExperienceChanged OnExperienceChanged;

private:
	// 현재 레벨(1부터 시작)에서 다음 레벨까지 필요한 경험치를 데이터테이블에서 찾아옴.
	// 테이블에 더 높은 레벨 행이 없으면(최종레벨) nullptr 반환 -> 그 이상은 레벨업 안 함.
	const FRGEXPLevelRow* FindCurrentLevelRow() const;

	// 경험치가 충분하면 레벨업 1회 처리 (초과 경험치는 다음 레벨로 이월). 레벨업했으면 true.
	bool TryLevelUpOnce();

	// 대기 중인 레벨업이 있으면 그중 하나를 실제로 UI에 띄움 (동시에 여러 장 안 띄우고 순차적으로)
	void PresentNextPendingLevelUpIfAny();

	UPROPERTY()
	UDataTable* ExperienceCurveTable = nullptr;

	int32 CurrentLevel = 1;
	float CurrentExperience = 0.f;

	// 한 번에 여러 레벨을 올랐을 때(연속 레벨업), 아직 선택 안 한 레벨업이 몇 개 남았는지
	int32 PendingLevelUpCount = 0;

	// 지금 UI에 강화 선택 카드가 떠 있는 중인지 (떠 있으면 다음 대기열을 바로 안 띄움)
	bool bIsPresentingUpgradeChoice = false;

	// 강화 ID별 선택 횟수. 최종 수치가 아니라 "몇 번 골랐는지"만 저장해서,
	// 선택 순서와 무관하게 항상 같은 최종 결과가 나오게 함.
	TMap<FName, int32> UpgradeStacks;

};