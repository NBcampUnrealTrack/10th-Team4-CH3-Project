#include "Gamemode/RGGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

// =========================================================
// Constructor
// =========================================================

ARGGameModeBase::ARGGameModeBase()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bStartRunSystem = true;

	CurrentState = ERunState::Init;

	RunDuration = 60.0f;
	RemainingTime = RunDuration;

	CurrentKills = 0;
	TargetKillsToClear = 10;
	CurrentScore = 0;

	bIsRunEnded = false;
	bIsStageCleared = false;

	bHasRunStarted = false;
	bIsRunReady = false;

	// UI
	bCreateDefaultUI = false;
	DefaultUIClass = nullptr;
	DefaultUIWidget = nullptr;
	DefaultUIZOrder = 0;

	// Input
	bApplyDefaultInputMode = true;
	DefaultInputMode = ERGInputMode::GameOnly;
	bShowMouseCursor = false;
}

// =========================================================
// BeginPlay
// =========================================================

void ARGGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	ResetRunData();

	// 필수 시스템 준비 전 입력 차단
	SetGameplayInputEnabled(false);

	if (bCreateDefaultUI)
	{
		CreateDefaultUI();
	}

	// StartRun은 BP에서 HUD / Weapon 준비 완료 후 호출
	UE_LOG(LogTemp, Log, TEXT("[RGGameMode] Waiting for required systems..."));
}

// =========================================================
// EndPlay
// =========================================================

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
// ResetRunData
// =========================================================

void ARGGameModeBase::ResetRunData()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
	}

	CurrentState = ERunState::Init;

	RemainingTime = RunDuration;

	CurrentKills = 0;
	CurrentScore = 0;

	bIsRunEnded = false;
	bIsStageCleared = false;

	bHasRunStarted = false;
	bIsRunReady = false;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Run data reset. Time=%.1f / Kills=%d / Score=%d"),
		RemainingTime,
		CurrentKills,
		CurrentScore
	);
}

// =========================================================
// StartRun
// =========================================================

void ARGGameModeBase::StartRun()
{
	// 중복 시작 방지
	if (bHasRunStarted)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] StartRun ignored. First request was already processed.")
		);

		return;
	}

	// 첫 요청 즉시 잠금
	bHasRunStarted = true;

	CurrentState = ERunState::Loading;

	SetGameplayInputEnabled(false);

	// 필수 시스템 검증
	if (!VerifySystems())
	{
		bIsRunReady = false;

		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] Run start BLOCKED. Required systems are missing.")
		);

		return;
	}

	bIsRunReady = true;

	CurrentState = ERunState::Init;

	if (bApplyDefaultInputMode)
	{
		ApplyDefaultInputSettings();
	}

	SetGameplayInputEnabled(true);

	ChangeRunState(ERunState::Combat);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Run READY. Combat started.")
	);
}

// =========================================================
// SetGameplayInputEnabled
// =========================================================

void ARGGameModeBase::SetGameplayInputEnabled(bool bEnabled)
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PlayerController =
		GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
	{
		return;
	}

	APawn* PlayerPawn =
		PlayerController->GetPawn();

	if (!PlayerPawn)
	{
		return;
	}

	if (bEnabled)
	{
		PlayerPawn->EnableInput(PlayerController);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[RGGameMode] Gameplay input ENABLED.")
		);
	}
	else
	{
		PlayerPawn->DisableInput(PlayerController);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[RGGameMode] Gameplay input DISABLED.")
		);
	}
}

// =========================================================
// ChangeRunState
// =========================================================

void ARGGameModeBase::ChangeRunState(ERunState NewState)
{
	if (!GetWorld())
	{
		return;
	}

	if (CurrentState == NewState)
	{
		return;
	}

	// Result 상태에서는 다른 상태로 돌아가지 않음
	if (CurrentState == ERunState::Result)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] State change ignored. Run is already in Result state.")
		);

		return;
	}

	CurrentState = NewState;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] State changed: %d"),
		static_cast<int32>(CurrentState)
	);

	FTimerManager& TimerManager =
		GetWorld()->GetTimerManager();

	if (CurrentState == ERunState::Combat)
	{
		if (!TimerManager.TimerExists(RunTimerHandle))
		{
			TimerManager.SetTimer(
				RunTimerHandle,
				this,
				&ARGGameModeBase::UpdateRunTimer,
				1.0f,
				true
			);
		}
		else if (TimerManager.IsTimerPaused(RunTimerHandle))
		{
			TimerManager.UnPauseTimer(RunTimerHandle);
		}
	}
	else
	{
		if (TimerManager.TimerExists(RunTimerHandle))
		{
			TimerManager.PauseTimer(RunTimerHandle);
		}
	}
}

// =========================================================
// UpdateRunTimer
// =========================================================

void ARGGameModeBase::UpdateRunTimer()
{
	if (!bIsRunReady)
	{
		return;
	}

	if (bIsRunEnded)
	{
		return;
	}

	if (CurrentState != ERunState::Combat)
	{
		return;
	}

	if (RemainingTime <= 0.0f)
	{
		return;
	}

	RemainingTime -= 1.0f;

	if (RemainingTime <= 0.0f)
	{
		RemainingTime = 0.0f;

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
		}

		CheckEndCondition(false, true);
	}
}

// =========================================================
// OnEnemyDied
// =========================================================

void ARGGameModeBase::OnEnemyDied()
{
	if (!bIsRunReady)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] Kill ignored - Run not ready")
		);

		return;
	}

	if (bIsRunEnded)
	{
		return;
	}

	if (CurrentState != ERunState::Combat)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] Kill ignored - Not Combat")
		);

		return;
	}

	CurrentKills++;
	CurrentScore += 100;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RGGameMode] Kill: %d / %d"),
		CurrentKills,
		TargetKillsToClear
	);

	if (CurrentKills >= TargetKillsToClear)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] TARGET KILLS REACHED")
		);

		CheckEndCondition(false, false);
	}
}

// =========================================================
// CheckStageClearCondition
// =========================================================

bool ARGGameModeBase::CheckStageClearCondition() const
{
	return CurrentKills >= TargetKillsToClear;
}

// =========================================================
// CheckEndCondition
// =========================================================

void ARGGameModeBase::CheckEndCondition(
	bool bIsPlayerDead,
	bool bIsTimeOut
)
{
	if (!bIsRunReady)
	{
		return;
	}

	if (bIsRunEnded)
	{
		return;
	}

	if (
		CurrentState == ERunState::Loading ||
		CurrentState == ERunState::Result
		)
	{
		return;
	}

	// Player Death
	if (bIsPlayerDead)
	{
		bIsRunEnded = true;

		ExecuteGameOver(EDeathReason::Killed);

		return;
	}

	// Stage Clear
	// 중요: 여기서는 bIsRunEnded를 true로 만들지 않는다.
	if (CheckStageClearCondition())
	{
		ExecuteStageClear();

		return;
	}

	// Time Out
	if (bIsTimeOut)
	{
		bIsRunEnded = true;

		ExecuteGameOver(EDeathReason::TimeOut);

		return;
	}
}

// =========================================================
// ExecuteGameOver
// =========================================================

void ARGGameModeBase::ExecuteGameOver(EDeathReason Reason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
	}

	SetGameplayInputEnabled(false);

	CurrentState = ERunState::Result;

	switch (Reason)
	{
	case EDeathReason::Killed:
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GAME OVER] Player died.")
		);

		break;
	}

	case EDeathReason::TimeOut:
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GAME OVER] Time out.")
		);

		break;
	}

	default:
		break;
	}
}

// =========================================================
// ExecuteStageClear
// =========================================================

void ARGGameModeBase::ExecuteStageClear()
{
	// 중복 Stage Clear 방지
	if (bIsStageCleared)
	{
		return;
	}

	bIsStageCleared = true;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
	}

	// 10킬은 최종 승리가 아니므로 입력 유지
	// SetGameplayInputEnabled(false); 호출하지 않음

	CurrentState = ERunState::RestHub;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[STAGE CLEAR] Target kill count reached. Portal activation requested.")
	);

	// Blueprint에 Stage Clear 알림
	OnStageClear();
}

// =========================================================
// ExecuteVictory
// =========================================================

void ARGGameModeBase::ExecuteVictory()
{
	// 준비되지 않은 Run에서는 승리 불가
	if (!bIsRunReady)
	{
		return;
	}

	// 이미 종료된 경우 중복 처리 방지
	if (bIsRunEnded)
	{
		return;
	}

	// 10킬 Stage Clear 이후에만 승리 가능
	if (!bIsStageCleared)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[VICTORY] Ignored. Stage has not been cleared yet.")
		);

		return;
	}

	bIsRunEnded = true;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);
	}

	CurrentState = ERunState::Result;

	SetGameplayInputEnabled(false);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[VICTORY] Player entered the portal.")
	);
}

// =========================================================
// VerifySystems
// =========================================================

bool ARGGameModeBase::VerifySystems()
{
	if (!GetWorld())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] World is NULL.")
		);

		return false;
	}

	APlayerController* PlayerController =
		GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] PlayerController not found.")
		);

		return false;
	}

	APawn* PlayerPawn =
		PlayerController->GetPawn();

	if (!PlayerPawn)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] Player Pawn not found.")
		);

		return false;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RGGameMode] Required systems verified.")
	);

	return true;
}

// =========================================================
// CreateDefaultUI
// =========================================================

UUserWidget* ARGGameModeBase::CreateDefaultUI()
{
	if (DefaultUIWidget)
	{
		return DefaultUIWidget;
	}

	if (!DefaultUIClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] DefaultUIClass is not assigned.")
		);

		return nullptr;
	}

	if (!GetWorld())
	{
		return nullptr;
	}

	APlayerController* PlayerController =
		GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RGGameMode] Cannot create UI. PlayerController not found.")
		);

		return nullptr;
	}

	DefaultUIWidget =
		CreateWidget<UUserWidget>(
			PlayerController,
			DefaultUIClass
		);

	if (!DefaultUIWidget)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGGameMode] Failed to create Default UI.")
		);

		return nullptr;
	}

	DefaultUIWidget->AddToViewport(DefaultUIZOrder);

	return DefaultUIWidget;
}

// =========================================================
// RemoveDefaultUI
// =========================================================

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
// ApplyDefaultInputSettings
// =========================================================

void ARGGameModeBase::ApplyDefaultInputSettings()
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PlayerController =
		GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor =
		bShowMouseCursor;

	switch (DefaultInputMode)
	{
	case ERGInputMode::GameOnly:
	{
		FInputModeGameOnly InputMode;

		PlayerController->SetInputMode(InputMode);

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

		PlayerController->SetInputMode(InputMode);

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

		PlayerController->SetInputMode(InputMode);

		break;
	}

	default:
		break;
	}
}