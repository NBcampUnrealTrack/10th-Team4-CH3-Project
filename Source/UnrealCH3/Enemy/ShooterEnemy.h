// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "ShooterEnemy.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCH3_API AShooterEnemy : public ABaseEnemy
{
	GENERATED_BODY()
	
public:
	AShooterEnemy();
	
	virtual void Attack() override;
};
