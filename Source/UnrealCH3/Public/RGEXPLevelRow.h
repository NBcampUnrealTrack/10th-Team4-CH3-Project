// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RGEXPLevelRow.generated.h"

/*
 RowName은 1 : 1 -> 2 레벨까지 필요한 경험치
 노션 표대로: 1→2=100, 2→3=140, 3→4=190, 4→5=250, 5→6=320
 */
USTRUCT(BlueprintType)
struct FRGEXPLevelRow : public FTableRowBase
{
	GENERATED_BODY()

	// 이 레벨에서 다음 레벨로 올라가기 위해 필요한 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	float RequiredExperience = 100.f;

	// 이 레벨업 때 UI에 띄울 강화 후보 카드 수 (기본 3장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	int32 UpgradeChoiceCount = 3;
};
