// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnPoint.generated.h"

// 적 클래스와 컴포넌트 포인터 선언에 사용할 전방 선언
class ABaseEnemy;
class UArrowComponent;
class USceneComponent;

// 적의 생성 위치와 낙사 복귀 위치를 제공하는 레벨 배치용 Actor
UCLASS()
class UNREALCH3_API AEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	// Tick과 컴포넌트, 충돌 여부를 초기화한다.
	AEnemySpawnPoint();

	//스폰 판단용
	// 포인트가 활성화되어 있고 스폰 용도로 허용되었는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool CanBeUsedForSpawn() const;

	//복귀 판단용
	// 포인트가 활성화되어 있고 낙사 복귀 용도로 허용되었는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool CanBeUsedForRecovery() const;

	//땅에 안곂치게 스폰하는 용도
	// 적 캡슐의 반높이와 바닥 여유 높이를 적용한 생성 Transform을 반환한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	FTransform GetEnemySpawnTransform(TSubclassOf<ABaseEnemy> EnemyClass) const;

	//스폰 시 플레이어와의 최소 거리 조건 검사
	// 플레이어와 포인트 사이의 거리가 MinimumPlayerDistance 이상인지 확인한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Point")
	bool PassesSpawnDistanceCheck(const AActor* PlayerActor) const;

	// 포인트의 스폰 및 복귀 기능을 전체적으로 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn Point")
	bool bEnabled = true;

	// 이 포인트를 적 생성 위치로 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn Point")
	bool bUSeForSpawn = true;

	// 이 포인트를 낙사 또는 이동 실패 후 복귀 위치로 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn Point")
	bool bUseForRecovery = true;

	// 스폰 전에 플레이어와의 최소 거리를 검사할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn Point")
	bool bCheckMinimumPlayerDistance = true;

	// 적을 생성할 수 있는 플레이어와 포인트 사이의 최소 거리이다.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Enemy|Spawn Point", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bCheckMinimumPlayerDistance"))
	float MinimumPlayerDistance = 600.0f;

	// 적 캡슐이 지면과 너무 밀착되지 않도록 추가할 높이이다.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Enemy|Spawn Point", meta = (ClampMin = "0.0", Units = "cm"))
	float GroundClearance = 2.0f;

protected:
	// 스폰 포인트의 위치와 회전 기준이 되는 루트 컴포넌트이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// 에디터에서 적이 생성된 후 바라볼 방향을 표시하는 화살표이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> FacingArrow;


};
