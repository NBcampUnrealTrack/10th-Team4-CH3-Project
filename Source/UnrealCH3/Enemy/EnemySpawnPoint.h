// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnPoint.generated.h"

class ABaseEnemy;
class UArrowComponent;
class USceneComponent;

UCLASS()
class UNREALCH3_API AEnemySpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawnPoint();

	//스폰 판단용
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool CanBeUsedForSpawn() const;

	//복귀 판단용
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool CanBeUsedForRecovery() const;

	//땅에 안곂치게 스폰하는 용도
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	FTransform GetEnemySpawnTransform(TSubclassOf<ABaseEnemy> EnemyClass) const;

	//스폰 시 플레이어와의 최소 거리 조건 검사
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool PassesSpawnDistanceCheck(const AActor* PlayerActor) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> FacingArrow;


};
