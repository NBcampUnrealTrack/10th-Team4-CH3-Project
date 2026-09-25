#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gamemode/DataTableStruct/RGRunConfigRows.h"
#include "RGGameFlowBridge.generated.h"


class ARGGameModeBase;
class AEnemySpawnManager;
class ABaseEnemy;


UCLASS(Blueprintable)
class UNREALCH3_API ARGGameFlowBridge : public AActor
{
	GENERATED_BODY()


public:

	ARGGameFlowBridge();


protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;


	/**
	 * GameMode와 현재 레벨의 모든 EnemySpawnManager를 찾아
	 * Delegate를 연결한다.
	 */
	void InitializeBridge();


	/**
	 * 현재 레벨의 모든 SpawnManager가 관리하는
	 * 생존 적 수를 합산하여 GameMode에 전달한다.
	 */
	void UpdateCombinedAliveEnemyCount();


	/**
	 * SpawnManager에서 적 사망 이벤트 발생 시 호출된다.
	 *
	 * BossEnemy:
	 *		GameMode::OnBossKilled()
	 *
	 * 일반 Enemy:
	 *		GameMode::OnEnemyDied()
	 */
	UFUNCTION()
	void HandleEnemyKilled(
		ABaseEnemy* DeadEnemy
	);


	/**
	 * 어떤 SpawnManager의 생존 적 수가 바뀌었을 때 호출된다.
	 *
	 * SpawnManager가 여러 개 존재할 수 있으므로
	 * 전달받은 값을 그대로 쓰지 않고 전체를 다시 합산한다.
	 */
	UFUNCTION()
	void HandleAliveEnemyCountChanged(
		int32 AliveEnemyCount
	);


	/**
	 * GameMode의 RunFlow 상태가 실제로 변경됐을 때만
	 * 모든 SpawnManager에 AI 상태를 적용한다.
	 *
	 * BeginPlay 초기화 과정에서는 강제로 호출하지 않는다.
	 */
	UFUNCTION()
	void HandleRunFlowPolicyChanged(
		ERGRunInputPolicy InputPolicy,
		ERGRunTimePolicy TimePolicy,
		ERGRunAIState AIState,
		FName TopUI
	);


protected:

	/**
	 * 현재 레벨에서 사용하는 RGGameModeBase.
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "Game Flow|Reference"
	)
	TObjectPtr<ARGGameModeBase> GameModeRef;


	/**
	 * 현재 레벨에 존재하는 모든 EnemySpawnManager.
	 *
	 * 예:
	 * - 일반 잡몹 SpawnManager
	 * - Boss SpawnManager
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "Game Flow|Reference"
	)
	TArray<TObjectPtr<AEnemySpawnManager>> SpawnManagerRefs;
};