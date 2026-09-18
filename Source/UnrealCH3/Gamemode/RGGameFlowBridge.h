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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void InitializeBridge();

    UFUNCTION()
    void HandleEnemyKilled(ABaseEnemy* DeadEnemy);

    // [추가] SpawnManager 생존 수 -> GameMode Objective
    UFUNCTION()
    void HandleAliveEnemyCountChanged(int32 AliveEnemyCount);

    // [추가] GameMode RunFlow AIState -> SpawnManager
    UFUNCTION()
    void HandleRunFlowPolicyChanged(
        ERGRunInputPolicy InputPolicy,
        ERGRunTimePolicy TimePolicy,
        ERGRunAIState AIState,
        FName TopUI
    );

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Game Flow|Reference")
    TObjectPtr<ARGGameModeBase> GameModeRef;

    UPROPERTY(BlueprintReadOnly, Category = "Game Flow|Reference")
    TObjectPtr<AEnemySpawnManager> SpawnManagerRef;
};
