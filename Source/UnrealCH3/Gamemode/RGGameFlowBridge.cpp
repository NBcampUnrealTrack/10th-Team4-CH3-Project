#include "Gamemode/RGGameFlowBridge.h"

#include "Gamemode/RGGameModeBase.h"
#include "Enemy/EnemySpawnManager.h"
#include "Enemy/BaseEnemy.h"
// [추가] Boss 전용 Objective 판정
#include "Enemy/BossEnemy.h"

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

        SpawnManagerRef->OnAliveEnemyCountChanged.RemoveDynamic(
            this,
            &ARGGameFlowBridge::HandleAliveEnemyCountChanged
        );
    }

    if (IsValid(GameModeRef))
    {
        GameModeRef->OnRunFlowPolicyChanged.RemoveDynamic(
            this,
            &ARGGameFlowBridge::HandleRunFlowPolicyChanged
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

    // [추가] 현재 생존 적 수를 Objective 시스템으로 전달
    SpawnManagerRef->OnAliveEnemyCountChanged.RemoveDynamic(
        this,
        &ARGGameFlowBridge::HandleAliveEnemyCountChanged
    );

    SpawnManagerRef->OnAliveEnemyCountChanged.AddDynamic(
        this,
        &ARGGameFlowBridge::HandleAliveEnemyCountChanged
    );

    // [추가] RunFlow AI 정책을 SpawnManager에 전달
    GameModeRef->OnRunFlowPolicyChanged.RemoveDynamic(
        this,
        &ARGGameFlowBridge::HandleRunFlowPolicyChanged
    );

    GameModeRef->OnRunFlowPolicyChanged.AddDynamic(
        this,
        &ARGGameFlowBridge::HandleRunFlowPolicyChanged
    );

    // BeginPlay 순서와 관계없이 현재 정책/현재 생존 수를 즉시 동기화
    SpawnManagerRef->ApplyRunAIState(
        GameModeRef->GetCurrentAIState()
    );

    GameModeRef->SetRemainingEnemyCount(
        SpawnManagerRef->GetCurrentAliveCount()
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

    // [추가] Boss는 일반 Kill Objective가 아니라 BossDefeat Objective로 보낸다.
    if (DeadEnemy && DeadEnemy->IsA<ABossEnemy>())
    {
        GameModeRef->OnBossKilled();

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("=== GAME FLOW BRIDGE -> GameMode OnBossKilled CALLED ===")
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


// [추가] SpawnManager 생존 수 -> GameMode Objective
void ARGGameFlowBridge::HandleAliveEnemyCountChanged(
    int32 AliveEnemyCount
)
{
    if (!IsValid(GameModeRef))
    {
        return;
    }

    GameModeRef->SetRemainingEnemyCount(
        AliveEnemyCount
    );
}


// [추가] GameMode RunFlow -> SpawnManager AI/Spawn 정책
void ARGGameFlowBridge::HandleRunFlowPolicyChanged(
    ERGRunInputPolicy InputPolicy,
    ERGRunTimePolicy TimePolicy,
    ERGRunAIState AIState,
    FName TopUI
)
{
    if (!IsValid(SpawnManagerRef))
    {
        return;
    }

    SpawnManagerRef->ApplyRunAIState(
        AIState
    );
}
