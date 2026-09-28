// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGMissionStatusWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class UNREALCH3_API URGMissionStatusWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:
	//적 처치, 목표처치 임시값으로 테스트
	UFUNCTION(BlueprintCallable, Category = "UI|Mission")
	void ApplyMissionStatus
	(float RemainingSeconds, int32 CurrentKills, int32 RequiredKills);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "UI|Mission", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RemainingTime;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Mission", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KillObjective;

	// 기존 Widget Blueprint에 아직 Text가 없어도 컴파일이 깨지지 않도록 Optional로 둔다.
	// 표시하려면 WBP에 TextBlock을 추가하고 이름을 정확히 Text_CoreObjective로 지정한다.
	UPROPERTY(BlueprintReadOnly, Category = "UI|Mission", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CoreObjective;

	//경고 표시 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Mission")
	float TimeWarningThreshold = 30.f;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Mission")
	void OnTimerWarningChanged(bool bWarningActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Mission")
	void OnKillObjectiveCompletedChanged(bool bCompleted);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Mission")
	void OnCoreObjectiveCompletedChanged(bool bCompleted);

private:
	bool bHasReceivedStatus = false;
	bool bTimeWarningActive = false;

	bool bHasReceiveKillStatus = false;
	bool bKillObjectiveCompleted = false;

	bool bHasReceivedCoreStatus = false;
	bool bCoreObjectiveCompleted = false;
};
