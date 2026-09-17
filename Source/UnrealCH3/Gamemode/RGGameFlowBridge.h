#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Game Flow|Reference")
    TObjectPtr<ARGGameModeBase> GameModeRef;

    UPROPERTY(BlueprintReadOnly, Category = "Game Flow|Reference")
    TObjectPtr<AEnemySpawnManager> SpawnManagerRef;
};
