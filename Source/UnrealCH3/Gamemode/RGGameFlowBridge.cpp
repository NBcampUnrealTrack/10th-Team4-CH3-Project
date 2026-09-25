#include "RGGameFlowBridge.h"

#include "Gamemode/RGGameModeBase.h"

#include "Enemy/EnemySpawnManager.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/BossEnemy.h"

#include "Kismet/GameplayStatics.h"


ARGGameFlowBridge::ARGGameFlowBridge()
{
	PrimaryActorTick.bCanEverTick = false;

	GameModeRef = nullptr;
	SpawnManagerRefs.Empty();
}


void ARGGameFlowBridge::BeginPlay()
{
	Super::BeginPlay();


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"=== GAME FLOW BRIDGE BeginPlay "
			"| Name=%s ==="
		),
		*GetName()
	);


	InitializeBridge();
}


void ARGGameFlowBridge::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	// =====================================================
	// 모든 SpawnManager Delegate 연결 해제
	// =====================================================

	for (
		const TObjectPtr<AEnemySpawnManager>& SpawnManagerPtr
		: SpawnManagerRefs
		)
	{
		AEnemySpawnManager* SpawnManager =
			SpawnManagerPtr.Get();


		if (!IsValid(SpawnManager))
		{
			continue;
		}


		// Enemy Kill
		SpawnManager->OnEnemyKilled.RemoveDynamic(
			this,
			&ARGGameFlowBridge::HandleEnemyKilled
		);


		// Alive Count
		SpawnManager->OnAliveEnemyCountChanged.RemoveDynamic(
			this,
			&ARGGameFlowBridge::HandleAliveEnemyCountChanged
		);
	}


	// =====================================================
	// GameMode Delegate 연결 해제
	// =====================================================

	if (IsValid(GameModeRef))
	{
		GameModeRef->OnRunFlowPolicyChanged.RemoveDynamic(
			this,
			&ARGGameFlowBridge::HandleRunFlowPolicyChanged
		);
	}


	// =====================================================
	// 참조 초기화
	// =====================================================

	SpawnManagerRefs.Empty();
	GameModeRef = nullptr;


	Super::EndPlay(
		EndPlayReason
	);
}


void ARGGameFlowBridge::InitializeBridge()
{
	// =====================================================
	// 1. GameMode 찾기
	// =====================================================

	GameModeRef =
		Cast<ARGGameModeBase>(
			UGameplayStatics::GetGameMode(this)
		);


	if (!IsValid(GameModeRef))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"=== GAME FLOW BRIDGE FAILED "
				"| RGGameModeBase NOT FOUND ==="
			)
		);

		return;
	}


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"=== GAME FLOW BRIDGE "
			"GameMode Connected "
			"| Name=%s "
			"| Ptr=%p ==="
		),
		*GameModeRef->GetName(),
		GameModeRef.Get()
	);


	// =====================================================
	// 2. 현재 레벨의 모든 EnemySpawnManager 검색
	// =====================================================

	TArray<AActor*> FoundSpawnManagers;


	UGameplayStatics::GetAllActorsOfClass(
		this,
		AEnemySpawnManager::StaticClass(),
		FoundSpawnManagers
	);


	if (FoundSpawnManagers.Num() <= 0)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"=== GAME FLOW BRIDGE FAILED "
				"| EnemySpawnManager NOT FOUND ==="
			)
		);

		return;
	}


	SpawnManagerRefs.Empty();


	// =====================================================
	// 3. 모든 SpawnManager에 Delegate 연결
	// =====================================================

	for (AActor* FoundActor : FoundSpawnManagers)
	{
		AEnemySpawnManager* SpawnManager =
			Cast<AEnemySpawnManager>(
				FoundActor
			);


		if (!IsValid(SpawnManager))
		{
			continue;
		}


		// -------------------------------------------------
		// SpawnManager 등록
		// -------------------------------------------------

		SpawnManagerRefs.AddUnique(
			SpawnManager
		);


		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"=== GAME FLOW BRIDGE "
				"SpawnManager Found "
				"| Name=%s "
				"| Ptr=%p ==="
			),
			*SpawnManager->GetName(),
			SpawnManager
		);


		// -------------------------------------------------
		// Enemy Kill Delegate
		// -------------------------------------------------

		SpawnManager->OnEnemyKilled.RemoveDynamic(
			this,
			&ARGGameFlowBridge::HandleEnemyKilled
		);


		SpawnManager->OnEnemyKilled.AddDynamic(
			this,
			&ARGGameFlowBridge::HandleEnemyKilled
		);


		// -------------------------------------------------
		// Alive Count Delegate
		// -------------------------------------------------

		SpawnManager->OnAliveEnemyCountChanged.RemoveDynamic(
			this,
			&ARGGameFlowBridge::HandleAliveEnemyCountChanged
		);


		SpawnManager->OnAliveEnemyCountChanged.AddDynamic(
			this,
			&ARGGameFlowBridge::HandleAliveEnemyCountChanged
		);


		// -------------------------------------------------
		// 중요:
		//
		// 여기서 ApplyRunAIState()를 호출하지 않는다.
		//
		// Spawn 직후 Boss의 AIController / BehaviorTree가
		// 초기화되는 도중 PauseLogic / RestartLogic 등이
		// 호출되면 Boss AI 시작 과정과 충돌할 수 있다.
		//
		// 이후 실제 RunState가 변경됐을 때
		// HandleRunFlowPolicyChanged()에서만 처리한다.
		// -------------------------------------------------


		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"=== GAME FLOW BRIDGE "
				"SpawnManager BOUND "
				"| Manager=%s "
				"| OnEnemyKilled=%s "
				"| Alive=%d ==="
			),
			*SpawnManager->GetName(),
			SpawnManager->OnEnemyKilled.IsBound()
			? TEXT("TRUE")
			: TEXT("FALSE"),
			SpawnManager->GetCurrentAliveCount()
		);
	}


	// =====================================================
	// 4. GameMode RunFlow Delegate 연결
	// =====================================================

	GameModeRef->OnRunFlowPolicyChanged.RemoveDynamic(
		this,
		&ARGGameFlowBridge::HandleRunFlowPolicyChanged
	);


	GameModeRef->OnRunFlowPolicyChanged.AddDynamic(
		this,
		&ARGGameFlowBridge::HandleRunFlowPolicyChanged
	);


	// =====================================================
	// 5. 현재 생존 적 수만 동기화
	//
	// AI 상태는 여기서 강제 동기화하지 않는다.
	// =====================================================

	UpdateCombinedAliveEnemyCount();


	// =====================================================
	// 초기화 완료
	// =====================================================

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"=== GAME FLOW BRIDGE INITIALIZED "
			"| SpawnManager Count=%d ==="
		),
		SpawnManagerRefs.Num()
	);
}


void ARGGameFlowBridge::UpdateCombinedAliveEnemyCount()
{
	if (!IsValid(GameModeRef))
	{
		return;
	}


	int32 TotalAliveEnemyCount = 0;


	// =====================================================
	// 모든 SpawnManager 생존 적 수 합산
	// =====================================================

	for (
		const TObjectPtr<AEnemySpawnManager>& SpawnManagerPtr
		: SpawnManagerRefs
		)
	{
		AEnemySpawnManager* SpawnManager =
			SpawnManagerPtr.Get();


		if (!IsValid(SpawnManager))
		{
			continue;
		}


		TotalAliveEnemyCount +=
			SpawnManager->GetCurrentAliveCount();
	}


	// =====================================================
	// GameMode에 전체 생존 수 전달
	// =====================================================

	GameModeRef->SetRemainingEnemyCount(
		TotalAliveEnemyCount
	);


	UE_LOG(
		LogTemp,
		Verbose,
		TEXT(
			"=== GAME FLOW BRIDGE "
			"Alive Enemy Updated "
			"| Total=%d ==="
		),
		TotalAliveEnemyCount
	);
}


void ARGGameFlowBridge::HandleEnemyKilled(
	ABaseEnemy* DeadEnemy
)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"=== GAME FLOW BRIDGE "
			"Enemy Killed "
			"| Enemy=%s ==="
		),
		*GetNameSafe(DeadEnemy)
	);


	// =====================================================
	// GameMode 확인
	// =====================================================

	if (!IsValid(GameModeRef))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"=== GAME FLOW BRIDGE "
				"Enemy Killed FAILED "
				"| GameModeRef INVALID ==="
			)
		);

		return;
	}


	// =====================================================
	// DeadEnemy 확인
	// =====================================================

	if (!IsValid(DeadEnemy))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"=== GAME FLOW BRIDGE "
				"Enemy Killed FAILED "
				"| DeadEnemy INVALID ==="
			)
		);

		return;
	}


	// =====================================================
	// Boss 사망
	// =====================================================

	if (DeadEnemy->IsA<ABossEnemy>())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"=== GAME FLOW BRIDGE "
				"BOSS DETECTED "
				"| Enemy=%s ==="
			),
			*GetNameSafe(DeadEnemy)
		);


		// Boss Objective 완료 처리
		GameModeRef->OnBossKilled();


		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"=== GAME FLOW BRIDGE -> "
				"GameMode OnBossKilled CALLED ==="
			)
		);


		// Boss Kill은 일반 Kill Objective에
		// 중복으로 포함시키지 않는다.
		return;
	}


	// =====================================================
	// 일반 적 사망
	// =====================================================

	GameModeRef->OnEnemyDied();


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"=== GAME FLOW BRIDGE -> "
			"GameMode OnEnemyDied CALLED ==="
		)
	);
}


void ARGGameFlowBridge::HandleAliveEnemyCountChanged(
	int32 AliveEnemyCount
)
{
	// -----------------------------------------------------
	// 전달된 AliveEnemyCount는
	// 이벤트를 발생시킨 특정 SpawnManager의 값이다.
	//
	// 현재는 SpawnManager가 여러 개이므로
	// 해당 값을 GameMode에 바로 넣으면 안 된다.
	//
	// 모든 SpawnManager를 다시 조회해서 합산한다.
	// -----------------------------------------------------

	(void)AliveEnemyCount;


	UpdateCombinedAliveEnemyCount();
}


void ARGGameFlowBridge::HandleRunFlowPolicyChanged(
	ERGRunInputPolicy InputPolicy,
	ERGRunTimePolicy TimePolicy,
	ERGRunAIState AIState,
	FName TopUI
)
{
	// 현재 Bridge에서 직접 사용하는 것은 AIState뿐이다.
	(void)InputPolicy;
	(void)TimePolicy;
	(void)TopUI;


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"=== GAME FLOW BRIDGE "
			"RunFlow Changed "
			"| AIState=%d "
			"| SpawnManagerCount=%d ==="
		),
		static_cast<int32>(AIState),
		SpawnManagerRefs.Num()
	);


	// =====================================================
	// 실제 RunFlow가 변경됐을 때만
	// 모든 SpawnManager에 AI 상태를 적용한다.
	// =====================================================

	for (
		const TObjectPtr<AEnemySpawnManager>& SpawnManagerPtr
		: SpawnManagerRefs
		)
	{
		AEnemySpawnManager* SpawnManager =
			SpawnManagerPtr.Get();


		if (!IsValid(SpawnManager))
		{
			continue;
		}


		SpawnManager->ApplyRunAIState(
			AIState
		);


		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"=== GAME FLOW BRIDGE "
				"RunFlow AI Applied "
				"| Manager=%s "
				"| AIState=%d ==="
			),
			*SpawnManager->GetName(),
			static_cast<int32>(AIState)
		);
	}
}