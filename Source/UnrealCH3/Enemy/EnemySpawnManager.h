// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy/DataTableStruct/RGEnemySpawnData.h"
#include "EnemySpawnManager.generated.h"

// 포인터와 클래스 참조에 사용할 전방 선언
class ABaseEnemy;
class AEnemySpawnPoint;
class UDataTable;

// 스테이지별 스폰 예산과 현재 활성 적을 관리하는 레벨 배치용 Actor
UCLASS()
class UNREALCH3_API AEnemySpawnManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	// 스폰 매니저의 Tick 사용 여부와 기본 상태를 설정한다.
	AEnemySpawnManager();

protected:
	// Called when the game starts or when spawned
	// 레벨 시작 시 스폰 포인트를 찾고 자동 시작 여부에 따라 스폰을 시작한다.
	virtual void BeginPlay() override;

	// 레벨 종료 또는 매니저 제거 시 실행 중인 보충 타이머를 정리한다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	//맵 id에 해당하는 DT행으로 스폰 시작
	// StageId에 해당하는 예산 행을 읽고 초기 생성과 보충 타이머를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn")
	bool StartSpawning();

	//보충 타이머 중단
	// 현재 살아 있는 적은 유지하고 이후 보충 생성만 중단한다.
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn")
	void StopSpawning();

	// 스폰 시스템 활성화 상태를 변경하고 값에 따라 시작 또는 중지한다.
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn")
	void SetSpawningEnabled(bool bEnabled);

	//일반 사망과 무보상 오류 사망 모두 호출
	//무보상 사망 시 예산 환불이나 리워드는 주지 않는다.
	// 전달된 적을 현재 활성 적 목록에서 제거한다.
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn")
	void NotifyEnemyNoLongerActive(ABaseEnemy* Enemy);

	//살이있는 적 수 반환
	// 매니저가 현재 활성 상태로 추적하는 적의 수를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn")
	int32 GetCurrentAliveCount() const;

	//총 스폰 수 반환
	// 현재 스테이지에서 생성에 성공한 일반 적과 엘리트의 누적 수를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn")
	int32 GetTotalSpawnedCount() const;

	//총 스폰 엘리트 수 반환
	// 현재 스테이지에서 생성에 성공한 엘리트의 누적 수를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn")
	int32 GetTotalEliteSpawnedCount() const;

	//DT_SpawnBudget 지정
	// 스테이지별 스폰 예산 행이 저장된 DataTable을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn")
	TObjectPtr<UDataTable> SpawnBudgetTable;

	// SpawnBudgetTable에서 사용할 행 이름을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	FName StageId = NAME_None;

	// BeginPlay에서 스폰 시스템을 자동으로 시작할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	bool bAutoStart = true;

	// 보충 및 신규 생성이 가능한 현재 스폰 시스템 활성화 상태이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	bool bSpawningEnabled = true;

	// BeginPlay에서 레벨의 모든 AEnemySpawnPoint를 자동 검색할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	bool bAutoFindSpawnPoints = true;

	// 적 생성 후보로 사용할 스폰 포인트 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn")
	TArray<TObjectPtr<AEnemySpawnPoint>> SpawnPoints;

protected:
	// 보충 타이머가 실행될 때 활성화 상태를 확인하고 빈자리 보충을 요청한다.
	UFUNCTION()
	void HandleRefillTimer();

	// 매니저가 생성한 적 Actor가 Destroy되었을 때 활성 적 목록에서 제거한다.
	UFUNCTION()
	void HandleSpawnedEnemyDestroyed(AActor* DestroyedActor);

	// 레벨에 배치된 모든 AEnemySpawnPoint를 찾아 SpawnPoints에 저장한다.
	void FindSpawnPoints();

	// DataTable의 InitialComposition을 사용하여 최초 적 구성을 생성한다.
	bool SpawnInitialComposition();

	// 동시 최대 수 또는 한 번 생성 규칙에 따라 빈자리를 보충한다.
	bool FillToConcurrentLimit();

	// 전달된 적 클래스를 사용 가능한 스폰 포인트에서 실제로 생성한다.
	bool SpawnEnemy(TSubclassOf<ABaseEnemy> EnemyClass, bool bElite);

	// 유효한 보충 후보 중 하나를 가중치 기반 무작위 방식으로 선택한다.
	bool SelectRefillCandidate(FEnemyWeightedSpawnEntry& OutCandidate) const;

	// 사용 가능한 스폰 포인트 중 하나를 무작위로 선택한다.
	AEnemySpawnPoint* SelectSpawnPoint(TSubclassOf<ABaseEnemy> EnemyClass) const;

	// SpawnBudgetTable에서 StageId와 일치하는 예산 행을 반환한다.
	const FEnemySpawnBudgetRow* FindBudgetRow() const;

	// 전체 누적 생성 예산이 남아 있거나 무제한 상태인지 확인한다.
	bool HasTotalBudgetRemaining() const;

	// 누적 엘리트 생성 수가 엘리트 상한보다 작은지 확인한다.
	bool CanSpawnElite() const;

	// 전달된 적을 ActiveEnemies에서 제거하고 파괴 이벤트 연결을 해제한다.
	void RemoveActiveEnemy(ABaseEnemy* Enemy);

	// 매니저가 생성하고 현재 활성 상태로 추적 중인 적 목록이다.
	UPROPERTY(VisibleInstanceOnly, Category = "Enemy|Runtime")
	TSet<TObjectPtr<ABaseEnemy>> ActiveEnemies;

	// 일정 간격으로 보충 함수를 실행하기 위한 타이머 핸들이다.
	FTimerHandle RefillTimerHandle;

	// 초기 구성이 중복으로 생성되는 것을 방지하기 위한 상태값이다.
	bool bInitialSpawnCompleted = false;

	// 현재 스테이지에서 생성에 성공한 모든 적의 누적 수이다.
	int32 TotalSpawnedCount = 0;

	// 현재 스테이지에서 생성에 성공한 엘리트의 누적 수이다.
	int32 TotalEliteSpawnedCount = 0;
};
