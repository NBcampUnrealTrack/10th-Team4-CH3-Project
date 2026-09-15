// Fill out your copyright notice in the Description page of Project Settings.

#include "RGGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "RGProgressionSubsystem.h"


// =========================================================
// Constructor
// =========================================================

ARGGameModeBase::ARGGameModeBase()
{
	// -----------------------------------------------------
	// Tick ��� �� ��
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

	//시작할 때 URGProgressionSubsystem의 Progression 호출
	//만들어둔 경험치 테이블에 따라 경험치 커브 셋팅
	if (UGameInstance* GI = GetGameInstance()) {
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>()) {
			Progression->InitializeExperienceCurve(ExperienceCurveTable);
		}
	}

	// -----------------------------------------------------
	// 1. �ý��� ����
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
	// 2. �⺻ UI ����
	// -----------------------------------------------------

	if (bCreateDefaultUI)
	{
		CreateDefaultUI();
	}


	// -----------------------------------------------------
	// 3. �⺻ �Է� ����
	//
	// UIOnly�� ��� Widget�� ���� �����ؾ�
	// SetWidgetToFocus ��� ����
	// -----------------------------------------------------

	if (bApplyDefaultInputMode)
	{
		ApplyDefaultInputSettings();
	}


	// -----------------------------------------------------
	// 4. ���� Run �ý��� ����
	//
	// MainMenu GameMode��
	// bStartRunSystem = false �� ����
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
	// ���� ���¸� ����
	// -----------------------------------------------------

	if (CurrentState == NewState)
	{
		return;
	}


	// -----------------------------------------------------
	// Result / Loading ���¿�����
	// �Ϲ� ���� ���� ����
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
	// Combat �̿�
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
		 * MainMenu ���� ��Ȳ������
		 * BeginPlay ������ ���� ��� ���� �� �ֱ� ������
		 * ���⼭�� false ó������ �ʴ´�.
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
	// �̹� UI ����
	// -----------------------------------------------------

	if (DefaultUIWidget)
	{
		return DefaultUIWidget;
	}


	// -----------------------------------------------------
	// UI Class �̼���
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
	// PlayerController ã��
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
	// Widget ����
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
	// Viewport �߰�
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