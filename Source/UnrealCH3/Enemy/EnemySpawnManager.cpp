// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemySpawnManager.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Enemy/BaseEnemy.h"
#include "Enemy/EnemySpawnPoint.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

// Sets default values
AEnemySpawnManager::AEnemySpawnManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// 매 프레임 실행할 작업이 없으므로 Tick을 비활성화한다.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

}

// Called when the game starts or when spawned
void AEnemySpawnManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("=== SPAWN MANAGER BeginPlay | Name=%s | This=%p ==="),
		*GetName(),
		this
	);

	// 자동 검색이 활성화되어 있으면 레벨의 모든 스폰 포인트를 수집한다.
	if (bAutoFindSpawnPoints)
	{
		FindSpawnPoints();
	}

	// 자동 시작이 활성화되어 있으면 DataTable을 읽어 스폰 시스템을 시작한다.
	if (bAutoStart)
	{
		StartSpawning();
	}
}

// 레벨 종료 또는 매니저 제거 시 실행 중인 타이머를 정리한다.
void AEnemySpawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 유효한 월드가 존재하는 경우 보충 타이머를 제거한다.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefillTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// 현재 StageId에 해당하는 설정으로 초기 생성과 보충 타이머를 시작한다.
bool AEnemySpawnManager::StartSpawning()
{
	// 월드가 없으면 Actor 생성과 타이머 실행이 불가능하므로 시작하지 않는다.
	if (!GetWorld())
	{
		return false;
	}

	// 스폰 기능이 비활성화된 상태이면 시작하지 않는다.
	if (!bSpawningEnabled)
	{
		return false;
	}

	// DataTable에서 현재 StageId에 해당하는 스폰 예산 행을 찾는다.
	const FEnemySpawnBudgetRow* Budget = FindBudgetRow();

	// DataTable 또는 StageId가 유효하지 않으면 오류 로그를 출력하고 중단한다.
	if (!Budget)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[EnemySpawnManager] Spawn budget row not found. "
				"StageId: %s"
			),
			*StageId.ToString()
		);

		return false;
	}

	// 자동 검색이 활성화되어 있고 저장된 포인트가 없으면 포인트를 다시 검색한다.
	if (SpawnPoints.IsEmpty() && bAutoFindSpawnPoints)
	{
		FindSpawnPoints();
	}

	// 생성에 사용할 스폰 포인트가 없으면 오류 로그를 출력하고 중단한다.
	if (SpawnPoints.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[EnemySpawnManager] No EnemySpawnPoint found."
			)
		);

		return false;
	}

	// 아직 초기 생성이 수행되지 않았다면 DataTable의 초기 구성을 생성한다.
	if (!bInitialSpawnCompleted)
	{
		SpawnInitialComposition();
		// 이후 StartSpawning이 다시 호출되어도 초기 구성을 중복 생성하지 않도록 기록한다.
		bInitialSpawnCompleted = true;
	}

	// DataTable에 지정된 간격으로 HandleRefillTimer를 반복 호출한다.
	GetWorld()->GetTimerManager().SetTimer(
		RefillTimerHandle,
		this,
		&AEnemySpawnManager::HandleRefillTimer,
		Budget->RefillIntervalSeconds,
		true
	);

	// 스폰 시스템과 보충 타이머가 정상적으로 시작되었음을 반환한다.
	return true;
}

// 실행 중인 보충 타이머를 제거하여 추가 생성을 중지한다.
void AEnemySpawnManager::StopSpawning()
{
	// 현재 존재하는 적은 제거하지 않고 타이머만 중지한다.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefillTimerHandle);
	}
}

// 외부 시스템에서 스폰 활성화 상태를 변경한다.
void AEnemySpawnManager::SetSpawningEnabled(bool bEnabled)
{
	// 전달받은 활성화 상태를 저장한다.
	bSpawningEnabled = bEnabled;

	// 활성화 상태이면 스폰 시스템을 시작하거나 재개한다.
	if (bSpawningEnabled)
	{
		StartSpawning();
	}
	else
	{
		// 비활성화 상태이면 보충 타이머를 중지한다.
		StopSpawning();
	}
}

// 일반 사망 또는 오류 사망한 적을 현재 활성 적 목록에서 제거한다.
void AEnemySpawnManager::NotifyEnemyNoLongerActive(ABaseEnemy* Enemy)
{
	RemoveActiveEnemy(Enemy);
}

// 매니저가 현재 활성 상태로 추적하는 적 수를 반환한다.
int32 AEnemySpawnManager::GetCurrentAliveCount() const
{
	return ActiveEnemies.Num();
}

// 생성에 성공한 모든 적의 누적 수를 반환한다.
int32 AEnemySpawnManager::GetTotalSpawnedCount() const
{
	return TotalSpawnedCount;
}

// 생성에 성공한 엘리트의 누적 수를 반환한다.
int32 AEnemySpawnManager::GetTotalEliteSpawnedCount() const
{
	return TotalEliteSpawnedCount;
}

// SpawnBudgetTable에서 StageId와 일치하는 행을 찾는다.
const FEnemySpawnBudgetRow* AEnemySpawnManager::FindBudgetRow() const
{
	// DataTable이 지정되지 않았거나 StageId가 비어 있으면 행을 찾을 수 없다.
	if (!SpawnBudgetTable || StageId.IsNone())
	{
		return nullptr;
	}

	// 현재 StageId를 Row Name으로 사용하여 예산 구조체 포인터를 반환한다.
	return SpawnBudgetTable->FindRow<FEnemySpawnBudgetRow>(StageId, TEXT("EnemySpawnManager"));
}

// 전체 누적 생성 예산이 남아 있는지 확인한다.
bool AEnemySpawnManager::HasTotalBudgetRemaining() const
{
	// 현재 스테이지에 해당하는 예산 행을 찾는다.
	const FEnemySpawnBudgetRow* Budget = FindBudgetRow();

	// 예산 행을 찾지 못했다면 추가 생성이 불가능하다.
	if (!Budget)
	{
		return false;
	}

	// 무제한 예산이 활성화되어 있으면 누적 생성 수와 관계없이 생성 가능 상태를 반환한다.
	if (Budget->bUnlimitedTotalEnemyBudget)
	{
		return true;
	}

	// 현재 누적 생성 수가 전체 예산보다 작을 때만 추가 생성이 가능하다.
	return TotalSpawnedCount < Budget->TotalEnemyBudget;
}

// 엘리트 누적 생성 상한이 남아 있는지 확인한다.
bool AEnemySpawnManager::CanSpawnElite() const
{
	// 현재 스테이지에 해당하는 예산 행을 찾는다.
	const FEnemySpawnBudgetRow* Budget = FindBudgetRow();

	// 예산 행을 찾지 못했다면 엘리트를 생성할 수 없다.
	if (!Budget)
	{
		return false;
	}

	// 현재 누적 엘리트 생성 수가 설정된 상한보다 작을 때만 생성 가능하다.
	return TotalEliteSpawnedCount < Budget->TotalEliteSpawnLimit;
}

// 전달된 적을 현재 활성 적 목록에서 제거한다.
void AEnemySpawnManager::RemoveActiveEnemy(ABaseEnemy* Enemy)
{
	// 적 포인터가 유효하지 않으면 제거 작업을 수행하지 않는다.
	if (!IsValid(Enemy))
	{
		return;
	}

	// ActiveEnemies에서 적을 제거하고 실제 제거된 항목 수를 저장한다.
	const int32 RemovedCount = ActiveEnemies.Remove(Enemy);

	// 목록에 존재했던 적이 실제로 제거된 경우에만 후속 작업을 수행한다.
	if (RemovedCount > 0)
	{
		// [추가] 정상 사망 이벤트 연결도 해제하여 중복 처리를 방지한다.
		Enemy->OnEnemyDeath.RemoveDynamic(this, &AEnemySpawnManager::HandleEnemyDeath);

		// 명시적으로 비활성 처리된 적이 나중에 Destroy될 때 중복 처리되지 않도록 이벤트 연결을 해제한다.
		Enemy->OnDestroyed.RemoveDynamic(this, &AEnemySpawnManager::HandleSpawnedEnemyDestroyed);

		// 제거된 적 이름과 제거 후 현재 활성 적 수를 로그로 출력한다.
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[EnemySpawnManager] Enemy inactive: %s "
				"| Alive: %d"
			),
			*GetNameSafe(Enemy),
			ActiveEnemies.Num()
		);
	}
}

// [추가] BaseEnemy가 정상 사망했을 때 호출된다.
void AEnemySpawnManager::HandleEnemyDeath(ABaseEnemy* DeadEnemy)
{
	if (!IsValid(DeadEnemy))
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("=== SPAWN MANAGER HandleEnemyDeath | Manager=%s | This=%p | Enemy=%s | OnEnemyKilled.IsBound=%s ==="),
		*GetName(),
		this,
		*DeadEnemy->GetName(),
		OnEnemyKilled.IsBound() ? TEXT("TRUE") : TEXT("FALSE")
	);

	RemoveActiveEnemy(DeadEnemy);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("=== SPAWN MANAGER OnEnemyKilled BROADCAST | Manager=%s | This=%p | IsBound=%s ==="),
		*GetName(),
		this,
		OnEnemyKilled.IsBound() ? TEXT("TRUE") : TEXT("FALSE")
	);

	OnEnemyKilled.Broadcast(DeadEnemy);
}

bool AEnemySpawnManager::FindRecoveryTransform_Implementation(AActor* Requester, FTransform& OutRecoveryTransform)
{
	if (!IsValid(Requester))
	{
		return false;
	}

	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);

	AEnemySpawnPoint* SelectedPoint = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AEnemySpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		if (!SpawnPoint->CanBeUsedForRecovery())
		{
			continue;
		}

		const FVector PointLocation = SpawnPoint->GetActorLocation();

		if (IsValid(PlayerActor))
		{
			FVector DirectionToPoint = PointLocation - PlayerActor->GetActorLocation();

			DirectionToPoint.Z = 0.0f;

			if (DirectionToPoint.IsNearlyZero())
			{
				continue;
			}

			DirectionToPoint.Normalize();

			FVector PlayerForward = PlayerActor->GetActorForwardVector();

			PlayerForward.Z = 0.0f;

			if (!PlayerForward.Normalize())
			{
				continue;
			}

			const float DirectionDot = FVector::DotProduct(PlayerForward, DirectionToPoint);

			if (DirectionDot > 0.85f)
			{
				continue;
			}

			if (DirectionDot < -0.85f)
			{
				continue;
			}
		}

		const float DistanceSquared = FVector::DistSquared(Requester->GetActorLocation(), PointLocation);

		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			SelectedPoint = SpawnPoint;
		}

		if (!IsValid(SelectedPoint))
		{
			return false;
		}
	}

	OutRecoveryTransform = SelectedPoint->GetActorTransform();

	if (const ACharacter* Character = Cast<ACharacter>(Requester))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			FVector RecoveryLocation = OutRecoveryTransform.GetLocation();

			RecoveryLocation.Z += Capsule->GetScaledCapsuleHalfHeight();

			OutRecoveryTransform.SetLocation(RecoveryLocation);
		}
	}

	return true;
}


// DataTable에 설정된 보충 간격마다 호출된다.
void AEnemySpawnManager::HandleRefillTimer()
{
	// 스폰 시스템이 비활성화되어 있으면 보충하지 않는다.
	if (!bSpawningEnabled)
	{
		return;
	}

	// 현재 빈자리를 확인하고 설정에 따라 동시 최대 수까지 보충한다.
	FillToConcurrentLimit();
}

// 매니저가 생성한 적 Actor의 OnDestroyed 이벤트에서 호출된다.
void AEnemySpawnManager::HandleSpawnedEnemyDestroyed(AActor* DestroyedActor)
{
	// 파괴된 Actor를 ABaseEnemy로 변환한다.
	ABaseEnemy* DestroyedEnemy = Cast<ABaseEnemy>(DestroyedActor);

	// 변환된 적을 현재 활성 적 목록에서 제거한다.
	RemoveActiveEnemy(DestroyedEnemy);
}

// 레벨에 배치된 모든 AEnemySpawnPoint를 검색한다.
void AEnemySpawnManager::FindSpawnPoints()
{
	// 기존 포인트 목록을 비우고 현재 레벨 기준으로 다시 수집한다.
	SpawnPoints.Reset();

	// 월드가 없으면 Actor 검색이 불가능하므로 중단한다.
	if (!GetWorld())
	{
		return;
	}

	// GetAllActorsOfClass에서 찾은 Actor를 임시로 저장한다.
	TArray<AActor*> FoundActors;

	// 현재 월드에 존재하는 모든 AEnemySpawnPoint Actor를 찾는다.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemySpawnPoint::StaticClass(), FoundActors);

	// 검색된 Actor를 하나씩 확인한다.
	for (AActor* Actor : FoundActors)
	{
		// 검색된 Actor를 AEnemySpawnPoint로 변환한다.
		AEnemySpawnPoint* SpawnPoint = Cast<AEnemySpawnPoint>(Actor);

		// 변환된 포인트가 유효하면 실제 스폰 후보 목록에 추가한다.
		if (IsValid(SpawnPoint))
		{
			SpawnPoints.Add(SpawnPoint);
		}
	}
}

// DataTable에 지정된 초기 적 구성을 생성한다.
bool AEnemySpawnManager::SpawnInitialComposition()
{
	// 현재 StageId에 해당하는 스폰 예산 행을 찾는다.
	const FEnemySpawnBudgetRow* Budget = FindBudgetRow();

	// 예산 행을 찾지 못하면 초기 생성을 수행할 수 없다.
	if (!Budget)
	{
		return false;
	}

	// 초기 적 중 한 마리라도 생성에 성공했는지 기록한다.
	bool bSpawnedAny = false;

	// InitialComposition에 등록된 적 그룹을 순서대로 확인한다.
	for (const FEnemyInitialSpawnGroup& Group : Budget->InitialComposition)
	{
		// 적 클래스가 비어 있거나 생성 수가 0 이하인 그룹은 건너뛴다.
		if (!Group.EnemyClass || Group.Count <= 0)
		{
			continue;
		}

		// 그룹에 설정된 Count만큼 한 마리씩 생성한다.
		for (int32 index = 0; index < Group.Count; ++index)
		{
			// 현재 활성 적 수가 동시 최대 수에 도달하면 초기 생성을 종료한다.
			if (ActiveEnemies.Num() >= Budget->MaxConcurrentEnemies)
			{
				return bSpawnedAny;
			}

			// 전체 누적 생성 예산이 소진되면 초기 생성을 종료한다.
			if (!HasTotalBudgetRemaining())
			{
				return bSpawnedAny;
			}

			// 엘리트 그룹이지만 엘리트 누적 상한이 소진되었다면 해당 생성을 건너뛴다.
			if (Group.bElite && !CanSpawnElite())
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT(
						"[EnemySpawnManager] Initial elite skipped. "
						"Elite limit reached."
					)
				);

				continue;
			}

			// 적 클래스와 엘리트 여부를 전달하여 실제 Actor 생성을 요청한다.
			if (!SpawnEnemy(Group.EnemyClass, Group.bElite))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT(
						"[EnemySpawnManager] "
						"Initial spawn failed."
					)
				);

				continue;
			}

			// 한 마리 이상 생성에 성공했음을 기록한다.
			bSpawnedAny = true;
		}
	}

	// 초기 구성 중 적 생성 성공 여부를 반환한다.
	return bSpawnedAny;
}

// 현재 활성 적 수가 동시 최대 수에 도달할 때까지 빈자리를 보충한다.
bool AEnemySpawnManager::FillToConcurrentLimit()
{
	// 현재 StageId에 해당하는 스폰 예산 행을 찾는다.
	const FEnemySpawnBudgetRow* Budget = FindBudgetRow();

	// 예산 행을 찾지 못하면 보충할 수 없다.
	if (!Budget)
	{
		return false;
	}

	// 이번 보충 주기에 한 마리라도 생성했는지 기록한다.
	bool bSpawnedAny = false;

	// 현재 활성 적 수가 동시 최대 수보다 작은 동안 반복한다.
	while (ActiveEnemies.Num() < Budget->MaxConcurrentEnemies)
	{
		// 전체 누적 생성 예산이 소진되면 보충을 종료한다.
		if (!HasTotalBudgetRemaining())
		{
			break;
		}

		// 가중치 선택 결과를 저장할 임시 후보이다.
		FEnemyWeightedSpawnEntry Candidate;

		// 유효한 보충 후보를 선택하지 못하면 보충을 종료한다.
		if (!SelectRefillCandidate(Candidate))
		{
			break;
		}

		// 선택한 후보의 실제 생성에 실패하면 같은 주기의 보충을 종료한다.
		if (!SpawnEnemy(Candidate.EnemyClass, Candidate.bElite))
		{
			break;
		}

		// 이번 보충 주기에 한 마리 이상 생성했음을 기록한다.
		bSpawnedAny = true;

		// 한 번에 최대 수까지 채우지 않는 설정이면 한 마리를 생성한 뒤 종료한다.
		if (!Budget->bFillToConcurrentLimitOnRefill)
		{
			break;
		}
	}

	// 이번 보충 주기의 적 생성 성공 여부를 반환한다.
	return bSpawnedAny;
}

// 선택된 적 클래스를 사용 가능한 스폰 포인트 중 하나에서 생성한다.
bool AEnemySpawnManager::SpawnEnemy(TSubclassOf<ABaseEnemy> EnemyClass, bool bElite)
{
	// 월드 또는 적 클래스가 유효하지 않으면 생성하지 않는다.
	if (!GetWorld() || !EnemyClass)
	{
		return false;
	}

	// 전체 누적 생성 예산이 남아 있지 않으면 생성하지 않는다.
	if (!HasTotalBudgetRemaining())
	{
		return false;
	}

	// 엘리트 후보이지만 엘리트 누적 상한이 소진되었다면 생성하지 않는다.
	if (bElite && !CanSpawnElite())
	{
		return false;
	}

	// 첫 번째 로컬 플레이어의 Pawn을 찾아 거리 검사 기준으로 사용한다.
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);

	// 활성화 상태와 플레이어 거리 검사를 통과한 포인트를 저장한다.
	TArray<AEnemySpawnPoint*> CandidatePoints;

	// 매니저가 보관 중인 모든 스폰 포인트를 확인한다.
	for (AEnemySpawnPoint* SpawnPoint : SpawnPoints)
	{
		// 포인트가 유효하지 않으면 건너뛴다.
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		// 비활성화되었거나 스폰 용도가 아닌 포인트는 건너뛴다.
		if (!SpawnPoint->CanBeUsedForSpawn())
		{
			continue;
		}

		// 플레이어와의 최소 거리 조건을 통과하지 못한 포인트는 건너뛴다.
		if (!SpawnPoint->PassesSpawnDistanceCheck(PlayerActor))
		{
			continue;
		}

		// 모든 조건을 통과한 포인트를 실제 생성 후보에 추가한다.
		CandidatePoints.Add(SpawnPoint);
	}

	// 특정 포인트에 생성이 편중되지 않도록 후보 배열을 무작위 순서로 섞는다.
	for (int32 Index = CandidatePoints.Num() - 1; Index > 0; --Index)
	{
		// 현재 Index까지의 범위에서 교환할 인덱스를 무작위로 선택한다.
		const int32 SwapIndex = FMath::RandRange(0, Index);

		// 현재 항목과 선택한 항목의 위치를 교환한다.
		CandidatePoints.Swap(Index, SwapIndex);
	}

	// 무작위로 섞인 각 포인트에서 적 생성을 순서대로 시도한다.
	for (AEnemySpawnPoint* SpawnPoint : CandidatePoints)
	{
		// 적 캡슐 높이와 포인트 방향이 적용된 생성 Transform을 가져온다.
		const FTransform SpawnTransform = SpawnPoint->GetEnemySpawnTransform(EnemyClass);

		// Actor 생성 시 사용할 Owner와 충돌 처리 방식을 설정한다.
		FActorSpawnParameters SpawnParameters;
		// 생성된 적의 Owner를 현재 스폰 매니저로 지정한다.
		SpawnParameters.Owner = this;
		// 다른 Actor와 겹치는 위치에서는 강제로 생성하지 않도록 설정한다.
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

		// 지정된 클래스와 Transform을 사용하여 실제 적 Actor를 생성한다.
		ABaseEnemy* SpawnedEnemy = GetWorld()->SpawnActor<ABaseEnemy>(EnemyClass, SpawnTransform, SpawnParameters);

		// 충돌 또는 다른 원인으로 생성에 실패하면 다음 포인트에서 다시 시도한다.
		if (!IsValid(SpawnedEnemy))
		{
			continue;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("=== SPAWN MANAGER SpawnEnemy | Manager=%s | This=%p | Enemy=%s ==="),
			*GetName(),
			this,
			*GetNameSafe(SpawnedEnemy)
		);

		// 생성에 성공한 적을 현재 활성 적 목록에 등록한다.
		ActiveEnemies.Add(SpawnedEnemy);

		// 전체 누적 생성 수를 증가시킨다.
		++TotalSpawnedCount;

		// 생성된 적이 엘리트라면 누적 엘리트 생성 수도 증가시킨다.
		if (bElite)
		{
			++TotalEliteSpawnedCount;
		}

		// [추가] 정상 사망 시 SpawnManager가 처치 사실을 받을 수 있도록 연결한다.
		SpawnedEnemy->OnEnemyDeath.AddDynamic(this, &AEnemySpawnManager::HandleEnemyDeath);

		// 적이 Destroy될 때 활성 적 목록에서 자동 제거되도록 이벤트를 연결한다.
		SpawnedEnemy->OnDestroyed.AddDynamic(this, &AEnemySpawnManager::HandleSpawnedEnemyDestroyed);

		// 생성된 적과 현재 생존 수 및 누적 생성 수를 로그로 출력한다.
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[EnemySpawnManager] Spawned: %s ""| Alive: %d | Total: %d | Elite: %d"),
			*GetNameSafe(SpawnedEnemy),
			ActiveEnemies.Num(),
			TotalSpawnedCount,
			TotalEliteSpawnedCount);

		// 한 포인트에서 생성에 성공했으므로 추가 포인트 시도를 종료한다.
		return true;
	}

	return false;
}

// 유효한 보충 후보 중 하나를 가중치 기반으로 선택한다.
bool AEnemySpawnManager::SelectRefillCandidate(FEnemyWeightedSpawnEntry& OutCandidate) const
{
	// 현재 StageId에 해당하는 스폰 예산 행을 찾는다.
	const FEnemySpawnBudgetRow* Budget = FindBudgetRow();

	// 예산 행을 찾지 못하면 후보를 선택할 수 없다.
	if (!Budget)
	{
		return false;
	}

	// 클래스와 가중치 및 엘리트 조건을 통과한 후보 포인터를 저장한다.
	TArray<const FEnemyWeightedSpawnEntry*> ValidCandidates;
	// 유효 후보들의 전체 가중치 합계를 저장한다.
	float TotalWeight = 0.0f;

	// DataTable에 등록된 모든 보충 후보를 확인한다.
	for (const FEnemyWeightedSpawnEntry& Candidate : Budget->RefillCandidates)
	{
		// 적 클래스가 지정되지 않은 후보는 제외한다.
		if (!Candidate.EnemyClass)
		{
			continue;
		}

		// 가중치가 0 이하인 후보는 선택 가능성이 없으므로 제외한다.
		if (Candidate.SpawnWeight <= 0.0f)
		{
			continue;
		}

		// 엘리트 후보이지만 엘리트 누적 상한이 소진된 경우 제외한다.
		if (Candidate.bElite && !CanSpawnElite())
		{
			continue;
		}

		// 모든 조건을 통과한 후보를 선택 대상 목록에 추가한다.
		ValidCandidates.Add(&Candidate);
		// 후보의 가중치를 전체 가중치에 더한다.
		TotalWeight += Candidate.SpawnWeight;
	}

	// 유효 후보가 없거나 가중치 합계가 0 이하이면 선택에 실패한다.
	if (ValidCandidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return false;
	}

	// 0부터 전체 가중치 사이의 무작위 값을 생성한다.
	float SelectionValue = FMath::FRandRange(0.0f, TotalWeight);

	// 유효 후보의 가중치를 차례대로 빼면서 선택 구간을 찾는다.
	for (const FEnemyWeightedSpawnEntry* Candidate : ValidCandidates)
	{
		// 현재 후보가 차지하는 가중치만큼 무작위 값에서 차감한다.
		SelectionValue -= Candidate->SpawnWeight;

		// 값이 0 이하가 된 시점의 후보를 최종 결과로 선택한다.
		if (SelectionValue <= 0.0f)
		{
			// 포인터가 가리키는 후보 데이터를 출력 매개변수에 복사한다.
			OutCandidate = *Candidate;
			return true;
		}
	}

	// 부동소수점 경계 오차로 반복문에서 선택되지 않은 경우 마지막 후보를 선택한다.
	OutCandidate = *ValidCandidates.Last();
	// 마지막 후보가 선택되었으므로 성공 상태를 반환한다.
	return true;
}

// 사용 가능한 스폰 포인트 중 하나를 무작위로 반환한다.
AEnemySpawnPoint* AEnemySpawnManager::SelectSpawnPoint(TSubclassOf<ABaseEnemy> EnemyClass) const
{
	// 첫 번째 로컬 플레이어의 Pawn을 거리 검사 기준으로 가져온다.
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);

	// 활성화 상태와 거리 조건을 통과한 포인트를 저장한다.
	TArray<AEnemySpawnPoint*> ValidPoints;

	// 등록된 모든 스폰 포인트를 확인한다.
	for (AEnemySpawnPoint* SpawnPoint : SpawnPoints)
	{
		// 유효하지 않은 포인트는 제외한다.
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		// 비활성화되었거나 스폰 용도가 아닌 포인트는 제외한다.
		if (!SpawnPoint->CanBeUsedForSpawn())
		{
			continue;
		}

		// 플레이어와의 최소 거리 조건을 통과하지 못한 포인트는 제외한다.
		if (!SpawnPoint->PassesSpawnDistanceCheck(PlayerActor))
		{
			continue;
		}

		// 모든 조건을 통과한 포인트를 선택 대상에 추가한다.
		ValidPoints.Add(SpawnPoint);
	}

	// 사용 가능한 포인트가 없으면 null을 반환한다.
	if (ValidPoints.IsEmpty())
	{
		return nullptr;
	}

	// 유효 포인트 배열 범위에서 무작위 인덱스를 선택한다.
	const int32 RandomIndex = FMath::RandRange(0, ValidPoints.Num() - 1);

	// 무작위로 선택된 스폰 포인트를 반환한다.
	return ValidPoints[RandomIndex];
}
