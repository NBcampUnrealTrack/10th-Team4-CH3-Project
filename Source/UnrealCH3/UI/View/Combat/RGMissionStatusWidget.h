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
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RemainingTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KillObjective;

	
};
