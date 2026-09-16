// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "PursuerEnemy.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API APursuerEnemy : public ABaseEnemy
{
	GENERATED_BODY()

public:
	APursuerEnemy();
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Data")
	UDataTable* EnemyData;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Data")
	UDataTable* EnemyAttackData;
	
	virtual void Attack() override;
};
