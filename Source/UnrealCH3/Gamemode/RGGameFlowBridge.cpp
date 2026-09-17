#include "Gamemode/RGGameFlowBridge.h"

#include "Gamemode/RGGameModeBase.h"
#include "Enemy/EnemySpawnManager.h"
#include "Enemy/BaseEnemy.h"

#include "Kismet/GameplayStatics.h"

ARGGameFlowBridge::ARGGameFlowBridge()
{
    PrimaryActorTick.bCanEverTick = false;

    GameModeRef = nullptr;
    SpawnManagerRef = nullptr;
}

void ARGGameFlowBridge::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=== GAME FLOW BRIDGE BeginPlay | %s ==="),
        *GetName()
    );

    InitializeBridge();
}

void ARGGameFlowBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(SpawnManagerRef))
    {
        SpawnManagerRef->OnEnemyKilled.RemoveDynamic(
            this,
            &ARGGameFlowBridge::HandleEnemyKilled
        );
    }

    SpawnManagerRef = nullptr;
    GameModeRef = nullptr;

    Super::EndPlay(EndPlayReason);
}

void ARGGameFlowBridge::InitializeBridge()
{
    GameModeRef = Cast<ARGGameModeBase>(
        UGameplayStatics::GetGameMode(this)
    );

    if (!IsValid(GameModeRef))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("=== GAME FLOW BRIDGE FAILED : RGGameModeBase NOT FOUND ===")
        );
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=== GAME FLOW BRIDGE GameMode Connected | Name=%s | Ptr=%p ==="),
        *GameModeRef->GetName(),
        GameModeRef.Get()
    );

    AActor* FoundSpawnManager = UGameplayStatics::GetActorOfClass(
        this,
        AEnemySpawnManager::StaticClass()
    );

    SpawnManagerRef = Cast<AEnemySpawnManager>(FoundSpawnManager);

    if (!IsValid(SpawnManagerRef))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("=== GAME FLOW BRIDGE FAILED : EnemySpawnManager NOT FOUND ===")
        );
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=== GAME FLOW BRIDGE SpawnManager Found | Name=%s | Ptr=%p ==="),
        *SpawnManagerRef->GetName(),
        SpawnManagerRef.Get()
    );

    SpawnManagerRef->OnEnemyKilled.RemoveDynamic(
        this,
        &ARGGameFlowBridge::HandleEnemyKilled
    );

    SpawnManagerRef->OnEnemyKilled.AddDynamic(
        this,
        &ARGGameFlowBridge::HandleEnemyKilled
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=== GAME FLOW BRIDGE OnEnemyKilled BOUND | Manager=%s | IsBound=%s ==="),
        *SpawnManagerRef->GetName(),
        SpawnManagerRef->OnEnemyKilled.IsBound() ? TEXT("TRUE") : TEXT("FALSE")
    );
}

void ARGGameFlowBridge::HandleEnemyKilled(ABaseEnemy* DeadEnemy)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=== GAME FLOW BRIDGE Enemy Killed | Enemy=%s ==="),
        *GetNameSafe(DeadEnemy)
    );

    if (!IsValid(GameModeRef))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("=== GAME FLOW BRIDGE Enemy Killed FAILED : GameModeRef INVALID ===")
        );
        return;
    }

    GameModeRef->OnEnemyDied();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=== GAME FLOW BRIDGE -> GameMode OnEnemyDied CALLED ===")
    );
}
