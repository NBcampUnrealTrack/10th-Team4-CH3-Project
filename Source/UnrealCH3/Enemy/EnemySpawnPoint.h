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

	//���� �Ǵܿ�
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool CanBeUsedForSpawn() const;

	//���� �Ǵܿ�
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool CanBeUsedForRecovery() const;

	//���� �ȁ�ġ�� �����ϴ� �뵵
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	FTransform GetEnemySpawnTransform(TSubclassOf<ABaseEnemy> EnemyClass) const;

	//���� �� �÷��̾���� �ּ� �Ÿ� ���� �˻�
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool PassesSpawnDistanceCheck(const AActor* PlayerActor) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> FacingArrow;


};
