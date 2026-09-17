// Fill out your copyright notice in the Description page of Project Settings.

#include "RGGameModeBase.h"

#include "Gamemode/RGProgressionSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"


// =========================================================
// Constructor
// =========================================================

ARGGameModeBase::ARGGameModeBase()
{
	// -----------------------------------------------------
	// Tick
	// -----------------------------------------------------

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;


	// -----------------------------------------------------
	// GameMode
	// -----------------------------------------------------

	bStartRunSystem = true;


	// -----------------------------------------------------
	// Run State
	// -----------------------------------------------------

	CurrentState = ERunState::Init;

	RemainingTime = 60.0f;

	CurrentKills = 0;

	TargetKillsToClear = 10;

	bIsRunEnded = false;

	bIsStageCleared = false;


	// -----------------------------------------------------
	// UI
	// -----------------------------------------------------

	bCreateDefaultUI = false;

	DefaultUIClass = nullptr;

	DefaultUIWidget = nullptr;

	DefaultUIZOrder = 0;


	// -----------------------------------------------------
	// Input
	// -----------------------------------------------------

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


	// -----------------------------------------------------
	// Progression 초기화
	// -----------------------------------------------------

	if (UGameInstance* GI = GetGameInstance())
	{
		if (
			URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>()
			)
		{
			Progression->InitializeExperienceCurve(
				ExperienceCurveTable
			);

			Progression->InitializeGeneralUpgrades(
				GeneralUpgradeTable
			);

			Progression->InitializeCoreUpgrades(
				CoreUpgradeTable
			);
		}
	}


	// -----------------------------------------------------
	// 1. System Validation
	// -----------------------------------------------------

	if (!VerifySystems())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"System validation failed."
			)
		);

		return;
	}


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"System validation complete."
		)
	);


	// -----------------------------------------------------
	// 2. Default UI
	// -----------------------------------------------------

	if (bCreateDefaultUI)
	{
		CreateDefaultUI();
	}


	// -----------------------------------------------------
	// 3. Default Input
	// -----------------------------------------------------

	if (bApplyDefaultInputMode)
	{
		ApplyDefaultInputSettings();
	}


	// -----------------------------------------------------
	// 4. Run System Start
	// -----------------------------------------------------

	if (bStartRunSystem)
	{
		ChangeRunState(
			ERunState::Combat
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[RGGameMode] "
				"Run System disabled."
			)
		);
	}
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
		GetWorld()
			->GetTimerManager()
			.ClearTimer(
				RunTimerHandle
			);
	}


	RemoveDefaultUI();


	Super::EndPlay(
		EndPlayReason
	);
}


// =========================================================
// ChangeRunState
// =========================================================

void ARGGameModeBase::ChangeRunState(
	ERunState NewState
)
{
	if (!GetWorld())
	{
		return;
	}


	// -----------------------------------------------------
	// 동일 상태면 변경하지 않음
	// -----------------------------------------------------

	if (CurrentState == NewState)
	{
		return;
	}


	// -----------------------------------------------------
	// Result / Loading 상태에서는
	// 일반 상태 변경 차단
	// -----------------------------------------------------

	if (
		CurrentState == ERunState::Result ||
		CurrentState == ERunState::Loading
		)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"State transition ignored. "
				"CurrentState: %d / NewState: %d"
			),
			static_cast<int32>(
				CurrentState
				),
			static_cast<int32>(
				NewState
				)
		);

		return;
	}


	// -----------------------------------------------------
	// State 변경
	// -----------------------------------------------------

	const ERunState OldState =
		CurrentState;

	CurrentState =
		NewState;


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"State changed: %d -> %d"
		),
		static_cast<int32>(
			OldState
			),
		static_cast<int32>(
			CurrentState
			)
	);


	// -----------------------------------------------------
	// 외부 시스템에 State 변경 알림
	// -----------------------------------------------------

	OnRunStateChanged.Broadcast(
		OldState,
		CurrentState
	);


	FTimerManager& TimerManager =
		GetWorld()->GetTimerManager();


	// -----------------------------------------------------
	// Combat 진입
	// -----------------------------------------------------

	if (
		CurrentState ==
		ERunState::Combat
		)
	{
		if (
			!TimerManager.TimerExists(
				RunTimerHandle
			)
			)
		{
			TimerManager.SetTimer(
				RunTimerHandle,
				this,
				&ARGGameModeBase::UpdateRunTimer,
				1.0f,
				true
			);
		}
		else if (
			TimerManager.IsTimerPaused(
				RunTimerHandle
			)
			)
		{
			TimerManager.UnPauseTimer(
				RunTimerHandle
			);
		}
	}


	// -----------------------------------------------------
	// Combat 이외 상태
	// -----------------------------------------------------

	else
	{
		if (
			TimerManager.TimerExists(
				RunTimerHandle
			)
			)
		{
			TimerManager.PauseTimer(
				RunTimerHandle
			);
		}
	}
}


// =========================================================
// UpdateRunTimer
// =========================================================

void ARGGameModeBase::UpdateRunTimer()
{
	if (bIsRunEnded)
	{
		return;
	}


	if (
		CurrentState !=
		ERunState::Combat
		)
	{
		return;
	}


	if (RemainingTime <= 0.0f)
	{
		return;
	}


	// -----------------------------------------------------
	// 남은 시간 감소
	// -----------------------------------------------------

	RemainingTime -= 1.0f;

	RemainingTime = FMath::Max(
		RemainingTime,
		0.0f
	);


	// -----------------------------------------------------
	// 외부 시스템에 Timer 변경 알림
	// -----------------------------------------------------

	OnRemainingTimeChanged.Broadcast(
		RemainingTime
	);


	// -----------------------------------------------------
	// Time Out
	// -----------------------------------------------------

	if (RemainingTime <= 0.0f)
	{
		if (GetWorld())
		{
			GetWorld()
				->GetTimerManager()
				.ClearTimer(
					RunTimerHandle
				);
		}


		CheckEndCondition(
			false,
			true
		);
	}
}


// =========================================================
// CheckEndCondition
// =========================================================

void ARGGameModeBase::CheckEndCondition(
	bool bIsPlayerDead,
	bool bIsTimeOut
)
{
	if (bIsRunEnded)
	{
		return;
	}


	// -----------------------------------------------------
	// 1. Player Death
	// -----------------------------------------------------

	if (bIsPlayerDead)
	{
		bIsRunEnded = true;


		ExecuteGameOver(
			EDeathReason::Killed
		);


		return;
	}


	// -----------------------------------------------------
	// 2. Stage Clear
	// -----------------------------------------------------

	if (CheckStageClearCondition())
	{
		bIsRunEnded = true;


		ExecuteStageClear();


		return;
	}


	// -----------------------------------------------------
	// 3. Time Out
	// -----------------------------------------------------

	if (bIsTimeOut)
	{
		bIsRunEnded = true;


		ExecuteGameOver(
			EDeathReason::TimeOut
		);
	}
}


// =========================================================
// Stage Clear Condition
// =========================================================

bool ARGGameModeBase::CheckStageClearCondition() const
{
	return (
		CurrentKills >=
		TargetKillsToClear
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
			TEXT(
				"[RGGameMode] "
				"World is NULL."
			)
		);

		return false;
	}


	APlayerController* PlayerController =
		GetWorld()->GetFirstPlayerController();


	if (!PlayerController)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"PlayerController not found."
			)
		);


		/*
		 * MainMenu 등의 경우 BeginPlay 시점에
		 * PlayerController가 없을 수도 있으므로
		 * 여기서는 false 처리하지 않는다.
		 */
	}


	return true;
}


// =========================================================
// OnEnemyDied
// =========================================================

void ARGGameModeBase::OnEnemyDied()
{
	if (bIsRunEnded)
	{
		return;
	}


	if (
		CurrentState !=
		ERunState::Combat
		)
	{
		return;
	}


	// -----------------------------------------------------
	// Kill Count 증가
	// -----------------------------------------------------

	CurrentKills++;


	// -----------------------------------------------------
	// 외부 시스템에 Kill Count 변경 알림
	// -----------------------------------------------------

	OnKillCountChanged.Broadcast(
		CurrentKills,
		TargetKillsToClear
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"Kill: %d / %d"
		),
		CurrentKills,
		TargetKillsToClear
	);


	// -----------------------------------------------------
	// Stage Clear 확인
	// -----------------------------------------------------

	if (CheckStageClearCondition())
	{
		CheckEndCondition(
			false,
			false
		);
	}
}


// =========================================================
// ExecuteGameOver
// =========================================================

void ARGGameModeBase::ExecuteGameOver(
	EDeathReason Reason
)
{
	if (GetWorld())
	{
		GetWorld()
			->GetTimerManager()
			.ClearTimer(
				RunTimerHandle
			);
	}


	// -----------------------------------------------------
	// Result 상태 전환
	// -----------------------------------------------------

	ChangeRunState(
		ERunState::Result
	);


	// -----------------------------------------------------
	// 외부 시스템에 Game Over 알림
	// -----------------------------------------------------

	OnGameOver.Broadcast(
		Reason
	);


	// -----------------------------------------------------
	// Log
	// -----------------------------------------------------

	switch (Reason)
	{
	case EDeathReason::Killed:
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[GAME OVER] "
				"Player died."
			)
		);

		break;
	}


	case EDeathReason::TimeOut:
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[GAME OVER] "
				"Time out."
			)
		);

		break;
	}


	default:
	{
		break;
	}
	}
}


// =========================================================
// ExecuteStageClear
// =========================================================

void ARGGameModeBase::ExecuteStageClear()
{
	if (GetWorld())
	{
		GetWorld()
			->GetTimerManager()
			.ClearTimer(
				RunTimerHandle
			);
	}


	bIsStageCleared = true;


	// -----------------------------------------------------
	// RestHub 상태 전환
	// -----------------------------------------------------

	ChangeRunState(
		ERunState::RestHub
	);


	// -----------------------------------------------------
	// 외부 시스템에 Stage Clear 알림
	// -----------------------------------------------------

	OnStageCleared.Broadcast();


	// -----------------------------------------------------
	// 기존 Progression 처리
	// -----------------------------------------------------
	// 기존 팀원 작업을 건드리지 않기 위해 그대로 유지
	// PresentCoreUpgradeChoice = 핵심 강화 선택지 표시
	// -----------------------------------------------------

	if (UGameInstance* GI = GetGameInstance())
	{
		if (
			URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>()
			)
		{
			Progression->PresentCoreUpgradeChoice();
		}
	}


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[STAGE CLEAR] "
			"Stage clear condition met."
		)
	);
}


// =========================================================
// CreateDefaultUI
// =========================================================

UUserWidget* ARGGameModeBase::CreateDefaultUI()
{
	// -----------------------------------------------------
	// 이미 UI가 존재
	// -----------------------------------------------------

	if (DefaultUIWidget)
	{
		return DefaultUIWidget;
	}


	// -----------------------------------------------------
	// UI Class 미설정
	// -----------------------------------------------------

	if (!DefaultUIClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"DefaultUIClass is not assigned."
			)
		);

		return nullptr;
	}


	// -----------------------------------------------------
	// PlayerController 찾기
	// -----------------------------------------------------

	APlayerController* PlayerController =
		GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;


	if (!PlayerController)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"Cannot create UI. "
				"PlayerController not found."
			)
		);

		return nullptr;
	}


	// -----------------------------------------------------
	// Widget 생성
	// -----------------------------------------------------

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
			TEXT(
				"[RGGameMode] "
				"Failed to create Default UI."
			)
		);

		return nullptr;
	}


	// -----------------------------------------------------
	// Viewport 추가
	// -----------------------------------------------------

	DefaultUIWidget->AddToViewport(
		DefaultUIZOrder
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"Default UI created."
		)
	);


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


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"Default UI removed."
		)
	);
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
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"Cannot apply input settings. "
				"PlayerController not found."
			)
		);

		return;
	}


	// -----------------------------------------------------
	// Mouse Cursor
	// -----------------------------------------------------

	PlayerController->bShowMouseCursor =
		bShowMouseCursor;


	// -----------------------------------------------------
	// Input Mode
	// -----------------------------------------------------

	switch (DefaultInputMode)
	{
	case ERGInputMode::GameOnly:
	{
		FInputModeGameOnly InputMode;


		PlayerController->SetInputMode(
			InputMode
		);


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


		PlayerController->SetInputMode(
			InputMode
		);


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


		PlayerController->SetInputMode(
			InputMode
		);


		break;
	}


	default:
	{
		break;
	}
	}
}