#include "Gamemode/RGGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#include "Player/RGCharacter.h"
#include "Enemy/BaseEnemy.h"


ARGGameModeBase::ARGGameModeBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bStartRunSystem = true;

	CurrentState = ERunState::Init;

	bSystemsReady = false;
	bCombatStarted = false;
	bIsRunEnded = false;
	bIsStageCleared = false;

	RunTimeLimit = 150.0f;
	RemainingTime = RunTimeLimit;

	TargetKillsToClear = 10;
	TargetCoresToClear = 1;

	CurrentKills = 0;
	CurrentCores = 0;
	AliveEnemyCount = 0;
	CurrentScore = 0;

	bCreateDefaultUI = false;
	DefaultUIClass = nullptr;
	DefaultUIWidget = nullptr;
	DefaultUIZOrder = 0;

	bApplyDefaultInputMode = true;
	DefaultInputMode = ERGInputMode::GameOnly;
	bShowMouseCursor = false;
}


void ARGGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (bCreateDefaultUI)
	{
		CreateDefaultUI();
	}

	if (bApplyDefaultInputMode)
	{
		ApplyDefaultInputSettings();
	}

	if (!bStartRunSystem)
	{
		return;
	}

	InitializeRun();
}


void ARGGameModeBase::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
	}

	RemoveDefaultUI();

	Super::EndPlay(EndPlayReason);
}


// =========================================================
// Initialize Run
// =========================================================

void ARGGameModeBase::InitializeRun()
{
	// 시작 연타 방지
	if (CurrentState == ERunState::Loading)
	{
		return;
	}

	CurrentState = ERunState::Init;

	bSystemsReady = false;
	bCombatStarted = false;
	bIsRunEnded = false;
	bIsStageCleared = false;

	RemainingTime = RunTimeLimit;

	CurrentKills = 0;
	CurrentCores = 0;
	AliveEnemyCount = 0;
	CurrentScore = 0;

	AliveEnemies.Empty();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
	}

	// 준비 전 입력 금지
	SetPlayerActionsAllowed(false);

	if (!VerifySystems())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] Required systems missing. Combat blocked.")
		);

		return;
	}

	bSystemsReady = true;

	// 필수 객체 준비 후 입력 허용
	SetPlayerActionsAllowed(true);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Run initialized. Waiting for combat zone.")
	);
}


// =========================================================
// Start Combat
// =========================================================

bool ARGGameModeBase::StartCombat()
{
	if (!bSystemsReady)
	{
		return false;
	}

	if (bCombatStarted)
	{
		return false;
	}

	if (bIsRunEnded)
	{
		return false;
	}

	if (CurrentState == ERunState::Loading)
	{
		return false;
	}

	bCombatStarted = true;

	ChangeRunState(ERunState::Combat);

	GetWorldTimerManager().SetTimer(
		RunTimerHandle,
		this,
		&ARGGameModeBase::UpdateRunTimer,
		1.0f,
		true
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Combat started. Time = %.0f"),
		RemainingTime
	);

	return true;
}


bool ARGGameModeBase::IsCombatActive() const
{
	return
		bSystemsReady &&
		bCombatStarted &&
		!bIsRunEnded &&
		CurrentState == ERunState::Combat;
}


// =========================================================
// State
// =========================================================

void ARGGameModeBase::ChangeRunState(ERunState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	if (bIsRunEnded &&
		NewState != ERunState::Result &&
		NewState != ERunState::RestHub)
	{
		return;
	}

	CurrentState = NewState;

	if (!GetWorld())
	{
		return;
	}

	FTimerManager& TimerManager =
		GetWorld()->GetTimerManager();

	if (CurrentState == ERunState::Combat)
	{
		if (TimerManager.IsTimerPaused(RunTimerHandle))
		{
			TimerManager.UnPauseTimer(RunTimerHandle);
		}
	}
	else
	{
		if (TimerManager.IsTimerActive(RunTimerHandle))
		{
			TimerManager.PauseTimer(RunTimerHandle);
		}
	}

	// Combat 이외 상태에서는 무기 행동 금지
	switch (CurrentState)
	{
	case ERunState::Combat:
		SetPlayerActionsAllowed(true);
		break;

	case ERunState::Pause:
	case ERunState::Upgrade:
	case ERunState::Result:
	case ERunState::Loading:
		SetPlayerActionsAllowed(false);
		break;

	default:
		break;
	}
}


// =========================================================
// Timer
// =========================================================

void ARGGameModeBase::UpdateRunTimer()
{
	if (!IsCombatActive())
	{
		return;
	}

	RemainingTime =
		FMath::Max(0.0f, RemainingTime - 1.0f);

	if (RemainingTime > 0.0f)
	{
		return;
	}

	/*
	 * P0 우선순위
	 *
	 * 사망 > 완료 > 시간초과
	 *
	 * 사망은 NotifyPlayerDeath에서 즉시 bIsRunEnded 처리.
	 * 여기서는 완료 조건을 먼저 검사한 뒤 timeout 처리.
	 */

	if (CheckStageClearCondition())
	{
		ExecuteStageClear();
		return;
	}

	ExecuteGameOver(EDeathReason::TimeOut);
}


// =========================================================
// Enemy
// =========================================================

void ARGGameModeBase::RegisterEnemySpawned(
	ABaseEnemy* Enemy
)
{
	if (!Enemy || bIsRunEnded)
	{
		return;
	}

	if (AliveEnemies.Contains(Enemy))
	{
		return;
	}

	AliveEnemies.Add(Enemy);
	AliveEnemyCount = AliveEnemies.Num();
}


void ARGGameModeBase::UnregisterEnemy(
	ABaseEnemy* Enemy
)
{
	if (!Enemy)
	{
		return;
	}

	if (!AliveEnemies.Contains(Enemy))
	{
		return;
	}

	AliveEnemies.Remove(Enemy);
	AliveEnemyCount = AliveEnemies.Num();

	if (!bIsRunEnded)
	{
		CheckObjectiveCompletion();
	}
}


void ARGGameModeBase::RegisterEnemyKilled(
	ABaseEnemy* Enemy,
	int32 ScoreValue
)
{
	if (!Enemy || bIsRunEnded)
	{
		return;
	}

	// 등록되지 않은 적이면 중복 사망/취소된 적으로 취급
	if (!AliveEnemies.Contains(Enemy))
	{
		return;
	}

	AliveEnemies.Remove(Enemy);

	AliveEnemyCount = AliveEnemies.Num();

	++CurrentKills;

	CurrentScore += FMath::Max(0, ScoreValue);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Kill %d/%d | Core %d/%d | Alive %d | Score %d"),
		CurrentKills,
		TargetKillsToClear,
		CurrentCores,
		TargetCoresToClear,
		AliveEnemyCount,
		CurrentScore
	);

	CheckObjectiveCompletion();
}


// =========================================================
// Core
// =========================================================

void ARGGameModeBase::RegisterCoreDestroyed()
{
	if (bIsRunEnded)
	{
		return;
	}

	++CurrentCores;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Core %d/%d"),
		CurrentCores,
		TargetCoresToClear
	);

	CheckObjectiveCompletion();
}


// =========================================================
// Objective
// =========================================================

bool ARGGameModeBase::CheckStageClearCondition() const
{
	return
		CurrentKills >= TargetKillsToClear &&
		CurrentCores >= TargetCoresToClear &&
		AliveEnemyCount <= 0;
}


void ARGGameModeBase::CheckObjectiveCompletion()
{
	if (bIsRunEnded)
	{
		return;
	}

	if (!bCombatStarted)
	{
		return;
	}

	if (CheckStageClearCondition())
	{
		ExecuteStageClear();
	}
}


// =========================================================
// Player Death
// =========================================================

void ARGGameModeBase::NotifyPlayerDeath()
{
	if (bIsRunEnded)
	{
		return;
	}

	// 사망이 가장 우선
	ExecuteGameOver(EDeathReason::Killed);
}


// =========================================================
// Result
// =========================================================

void ARGGameModeBase::ExecuteGameOver(
	EDeathReason Reason
)
{
	if (bIsRunEnded)
	{
		return;
	}

	bIsRunEnded = true;

	GetWorldTimerManager().ClearTimer(RunTimerHandle);

	SetPlayerActionsAllowed(false);

	CurrentState = ERunState::Result;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[GAME OVER] Reason = %d"),
		static_cast<int32>(Reason)
	);
}


void ARGGameModeBase::ExecuteStageClear()
{
	if (bIsRunEnded)
	{
		return;
	}

	bIsRunEnded = true;
	bIsStageCleared = true;

	GetWorldTimerManager().ClearTimer(RunTimerHandle);

	SetPlayerActionsAllowed(false);

	CurrentState = ERunState::RestHub;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[STAGE CLEAR] Kill/Core/Alive requirements completed.")
	);
}


// =========================================================
// Systems
// =========================================================

bool ARGGameModeBase::VerifySystems()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	APlayerController* PC =
		World->GetFirstPlayerController();

	if (!PC)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] PlayerController missing.")
		);

		return false;
	}

	ARGCharacter* Character =
		Cast<ARGCharacter>(PC->GetPawn());

	if (!Character)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] RGCharacter missing.")
		);

		return false;
	}

	// 기본 무기 생성까지 여기서 보장
	if (!Character->InitializeDefaultWeapon())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] Default weapon missing.")
		);

		return false;
	}

	return true;
}


void ARGGameModeBase::SetPlayerActionsAllowed(
	bool bAllowed
)
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PC =
		GetWorld()->GetFirstPlayerController();

	if (!PC)
	{
		return;
	}

	if (ARGCharacter* Character =
		Cast<ARGCharacter>(PC->GetPawn()))
	{
		Character->SetPlayerActionsAllowed(bAllowed);
	}
}


// =========================================================
// UI
// =========================================================

UUserWidget* ARGGameModeBase::CreateDefaultUI()
{
	if (DefaultUIWidget)
	{
		return DefaultUIWidget;
	}

	if (!DefaultUIClass)
	{
		return nullptr;
	}

	APlayerController* PC =
		GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;

	if (!PC)
	{
		return nullptr;
	}

	DefaultUIWidget =
		CreateWidget<UUserWidget>(
			PC,
			DefaultUIClass
		);

	if (DefaultUIWidget)
	{
		DefaultUIWidget->AddToViewport(DefaultUIZOrder);
	}

	return DefaultUIWidget;
}


void ARGGameModeBase::RemoveDefaultUI()
{
	if (!DefaultUIWidget)
	{
		return;
	}

	DefaultUIWidget->RemoveFromParent();
	DefaultUIWidget = nullptr;
}


// =========================================================
// Input Mode
// =========================================================

void ARGGameModeBase::ApplyDefaultInputSettings()
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PC =
		GetWorld()->GetFirstPlayerController();

	if (!PC)
	{
		return;
	}

	PC->bShowMouseCursor = bShowMouseCursor;

	switch (DefaultInputMode)
	{
	case ERGInputMode::GameOnly:
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		break;
	}

	case ERGInputMode::UIOnly:
	{
		FInputModeUIOnly InputMode;

		if (DefaultUIWidget)
		{
			InputMode.SetWidgetToFocus(
				DefaultUIWidget->TakeWidget()
			);
		}

		PC->SetInputMode(InputMode);
		break;
	}

	case ERGInputMode::GameAndUI:
	{
		FInputModeGameAndUI InputMode;

		if (DefaultUIWidget)
		{
			InputMode.SetWidgetToFocus(
				DefaultUIWidget->TakeWidget()
			);
		}

		PC->SetInputMode(InputMode);
		break;
	}
	}
}