// Fill out your copyright notice in the Description page of Project Settings.

#include "RGGameModeBase.h"

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
	// Tick 사용 안 함
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
	// 1. 시스템 검증
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
	// 2. 기본 UI 생성
	// -----------------------------------------------------

	if (bCreateDefaultUI)
	{
		CreateDefaultUI();
	}


	// -----------------------------------------------------
	// 3. 기본 입력 설정
	//
	// UIOnly의 경우 Widget을 먼저 생성해야
	// SetWidgetToFocus 사용 가능
	// -----------------------------------------------------

	if (bApplyDefaultInputMode)
	{
		ApplyDefaultInputSettings();
	}


	// -----------------------------------------------------
	// 4. 실제 Run 시스템 시작
	//
	// MainMenu GameMode는
	// bStartRunSystem = false 로 설정
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
	// 같은 상태면 무시
	// -----------------------------------------------------

	if (CurrentState == NewState)
	{
		return;
	}


	// -----------------------------------------------------
	// Result / Loading 상태에서는
	// 일반 상태 변경 방지
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


	CurrentState = NewState;


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"State changed: %d"
		),
		static_cast<int32>(
			CurrentState
			)
	);


	FTimerManager& TimerManager =
		GetWorld()->GetTimerManager();


	// -----------------------------------------------------
	// Combat
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
	// Combat 이외
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


	RemainingTime -= 1.0f;


	if (RemainingTime <= 0.0f)
	{
		RemainingTime = 0.0f;


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
	// 3. TimeOut
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
		 * MainMenu 등의 상황에서도
		 * BeginPlay 순서에 따라 잠시 없을 수 있기 때문에
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


	CurrentKills++;


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


	ChangeRunState(
		ERunState::Result
	);


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


	ChangeRunState(
		ERunState::RestHub
	);


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
	// 이미 UI 존재
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