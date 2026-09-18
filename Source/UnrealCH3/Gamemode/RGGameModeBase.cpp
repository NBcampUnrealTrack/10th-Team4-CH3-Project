// Fill out your copyright notice in the Description page of Project Settings.

#include "RGGameModeBase.h"

#include "Gamemode/RGProgressionSubsystem.h"
// [추가] 6개 Run Config DataTable Row Struct
#include "Gamemode/DataTableStruct/RGRunConfigRows.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
// [추가] DataTable 및 현재 Level 이름 조회
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
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
	// [추가] Stage Config
	// -----------------------------------------------------

	StageConfigTable = nullptr;

	// 현재 테스트 맵처럼 StageId와 LevelName이 아직 완전히 정리되지 않은 경우
	// Stage01을 fallback으로 사용한다.
	DefaultStageConfigRowName = TEXT("Stage01");

	CurrentStageConfigRowName = NAME_None;
	CurrentStageId = NAME_None;

	RequiredCoresToClear = 0;
	StageCompletionDestination = NAME_None;
	StageExclusiveMaterial = NAME_None;


	// -----------------------------------------------------
	// [추가] Score Config
	// -----------------------------------------------------

	ScoreConfigTable = nullptr;
	CurrentScore = 0;


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
	// [추가] Stage Config 적용
	// -----------------------------------------------------
	//
	// Run System을 사용하는 전투 GameMode라면
	// Combat 시작 전에 현재 맵의 DT_StageConfig를 먼저 읽는다.
	//
	// 실패해도 기존 Constructor 기본값(60초 / 10킬)으로
	// 계속 플레이할 수 있도록 BeginPlay 자체는 중단하지 않는다.
	// -----------------------------------------------------

	if (bStartRunSystem)
	{
		if (!ApplyStageConfigForCurrentMap())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT(
					"[RGGameMode] "
					"StageConfig was not applied. "
					"Using fallback values. "
					"Time=%.1f / TargetKills=%d"
				),
				RemainingTime,
				TargetKillsToClear
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
// [추가] ApplyStageConfigForCurrentMap
// =========================================================

bool ARGGameModeBase::ApplyStageConfigForCurrentMap()
{
	// -----------------------------------------------------
	// DataTable 유효성 확인
	// -----------------------------------------------------

	if (!IsValid(StageConfigTable))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"StageConfigTable is not assigned."
			)
		);

		return false;
	}


	// -----------------------------------------------------
	// Row Struct가 올바른지 확인
	// -----------------------------------------------------

	if (
		StageConfigTable->GetRowStruct() !=
		FRGStageConfigRow::StaticStruct()
		)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"StageConfigTable RowStruct mismatch. "
				"Expected FRGStageConfigRow."
			)
		);

		return false;
	}


	// -----------------------------------------------------
	// 현재 Unreal Level Asset 이름 확인
	// -----------------------------------------------------

	const FString CurrentLevelName =
		UGameplayStatics::GetCurrentLevelName(
			this,
			true
		);

	const FName CurrentLevelId(
		*CurrentLevelName
	);


	const FRGStageConfigRow* SelectedRow =
		nullptr;

	FName SelectedRowName =
		NAME_None;


	// -----------------------------------------------------
	// 1순위:
	// 현재 LevelName과 StageId가 같은 Row 검색
	// -----------------------------------------------------

	const TMap<FName, uint8*>& RowMap =
		StageConfigTable->GetRowMap();


	for (const TPair<FName, uint8*>& Pair : RowMap)
	{
		const FRGStageConfigRow* Candidate =
			reinterpret_cast<const FRGStageConfigRow*>(
				Pair.Value
				);


		if (!Candidate)
		{
			continue;
		}


		if (Candidate->StageId == CurrentLevelId)
		{
			SelectedRow = Candidate;
			SelectedRowName = Pair.Key;

			break;
		}
	}


	// -----------------------------------------------------
	// 2순위:
	// StageId가 아직 실제 LevelName과 다르면
	// DefaultStageConfigRowName 사용
	// -----------------------------------------------------

	if (
		!SelectedRow &&
		!DefaultStageConfigRowName.IsNone()
		)
	{
		SelectedRow =
			StageConfigTable->FindRow<FRGStageConfigRow>(
				DefaultStageConfigRowName,
				TEXT("ApplyStageConfigForCurrentMap"),
				false
			);


		if (SelectedRow)
		{
			SelectedRowName =
				DefaultStageConfigRowName;


			UE_LOG(
				LogTemp,
				Warning,
				TEXT(
					"[RGGameMode] "
					"No StageId matched current Level '%s'. "
					"Using fallback row '%s'."
				),
				*CurrentLevelName,
				*DefaultStageConfigRowName.ToString()
			);
		}
	}


	if (!SelectedRow)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"No StageConfig row found for Level '%s'."
			),
			*CurrentLevelName
		);

		return false;
	}


	// -----------------------------------------------------
	// StageConfig -> GameMode 실제 값 적용
	// -----------------------------------------------------

	CurrentStageConfigRowName =
		SelectedRowName;

	CurrentStageId =
		SelectedRow->StageId;


	// 제한 시간
	RemainingTime =
		FMath::Max(
			0.0f,
			SelectedRow->TimeLimitSeconds
		);


	// 요구 Kill
	TargetKillsToClear =
		FMath::Max(
			0,
			SelectedRow->RequiredKills
		);


	// 이후 ObjectiveConfig 연동용 값도 함께 보관
	RequiredCoresToClear =
		FMath::Max(
			0,
			SelectedRow->RequiredCores
		);


	// 이후 PortalConfig 연동용
	StageCompletionDestination =
		SelectedRow->CompletionDestination;


	// 이후 보상/Inventory 연동용
	StageExclusiveMaterial =
		SelectedRow->ExclusiveMaterial;


	// 현재 Kill Count는 새로운 Stage 시작값 0
	CurrentKills = 0;


	// -----------------------------------------------------
	// 이미 Delegate를 듣고 있는 UI가 있다면 즉시 갱신
	// -----------------------------------------------------

	OnRemainingTimeChanged.Broadcast(
		RemainingTime
	);

	OnKillCountChanged.Broadcast(
		CurrentKills,
		TargetKillsToClear
	);


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGGameMode] "
			"StageConfig applied. "
			"Row=%s / StageId=%s / "
			"Time=%.1f / Kills=%d / Cores=%d / "
			"Destination=%s / Material=%s"
		),
		*CurrentStageConfigRowName.ToString(),
		*CurrentStageId.ToString(),
		RemainingTime,
		TargetKillsToClear,
		RequiredCoresToClear,
		*StageCompletionDestination.ToString(),
		*StageExclusiveMaterial.ToString()
	);


	return true;
}



// =========================================================
// [추가] GetScoreValue
// =========================================================

int32 ARGGameModeBase::GetScoreValue(
	FName ScoreEventId
) const
{
	if (
		ScoreEventId.IsNone() ||
		!IsValid(ScoreConfigTable)
		)
	{
		return 0;
	}


	if (
		ScoreConfigTable->GetRowStruct() !=
		FRGScoreConfigRow::StaticStruct()
		)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"ScoreConfigTable RowStruct mismatch. "
				"Expected FRGScoreConfigRow."
			)
		);

		return 0;
	}


	const FRGScoreConfigRow* ScoreRow =
		ScoreConfigTable->FindRow<FRGScoreConfigRow>(
			ScoreEventId,
			TEXT("GetScoreValue"),
			false
		);


	if (!ScoreRow)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"ScoreConfig row not found: %s"
			),
			*ScoreEventId.ToString()
		);

		return 0;
	}


	return ScoreRow->BaseScore;
}


// =========================================================
// [추가] AddScoreEvent
// =========================================================

int32 ARGGameModeBase::AddScoreEvent(
	FName ScoreEventId
)
{
	const int32 AddedScore =
		GetScoreValue(
			ScoreEventId
		);


	// 0점 이벤트도 정상 데이터일 수 있다.
	// 예: TrainingTargetHit, ForcedEnemyRemoval
	if (AddedScore == 0)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT(
				"[RGGameMode] "
				"Score event '%s' added 0 points."
			),
			*ScoreEventId.ToString()
		);

		return 0;
	}


	CurrentScore += AddedScore;


	// BP/HUD에 점수 변경 알림
	OnScoreChanged.Broadcast(
		CurrentScore,
		AddedScore
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"Score Event=%s / +%d / Total=%d"
		),
		*ScoreEventId.ToString(),
		AddedScore,
		CurrentScore
	);


	return AddedScore;
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
	// [수정]
	// RequiredKills == 0은 "KillCount가 이 스테이지의 완료 조건이 아님"으로 처리한다.
	// 예: Stage04_Boss는 추후 BossKilled Objective로 완료 판정을 연결한다.
	return (
		TargetKillsToClear > 0 &&
		CurrentKills >= TargetKillsToClear
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
	// [추가] DT_ScoreConfig 기반 일반 적 처치 점수 적용
	// -----------------------------------------------------
	//
	// 기존처럼 +100을 코드에 직접 쓰지 않고
	// DT_ScoreConfig의 NormalEnemyKilled Row를 사용한다.
	// 점수 조정은 DataTable 값만 바꾸면 된다.
	// -----------------------------------------------------

	AddScoreEvent(
		TEXT("NormalEnemyKilled")
	);


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