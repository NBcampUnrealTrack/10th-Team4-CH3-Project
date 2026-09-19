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
// [추가] GameMode-only 숫자키 무기 전환
#include "Player/RGCharacter.h"
#include "RGBaseWeapon.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"


// =========================================================
// Constructor
// =========================================================

ARGGameModeBase::ARGGameModeBase()
{
	// -----------------------------------------------------
	// Tick
	// -----------------------------------------------------

	// [수정]
	// 숫자 1/2/3 직접 감지를 위해 GameMode Tick을 사용한다.
	// 단순 키 입력 3개만 검사하므로 현재 싱글플레이 프로토타입에서는 부담이 매우 작다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;


	// -----------------------------------------------------
	// GameMode
	// -----------------------------------------------------

	bStartRunSystem = true;


	// -----------------------------------------------------
	// [추가] GameMode Weapon Switch
	// -----------------------------------------------------

	bEnableNumberKeyWeaponSwitch = true;

	RifleWeaponClass = nullptr;
	ShotgunWeaponClass = nullptr;
	RailgunWeaponClass = nullptr;


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
	// [추가] Run Flow Config
	// -----------------------------------------------------

	RunFlowConfigTable = nullptr;

	// 기존 기본 동작과 동일한 초기값
	CurrentInputPolicy = ERGRunInputPolicy::Gameplay;
	CurrentTimePolicy = ERGRunTimePolicy::Paused;
	CurrentAIState = ERGRunAIState::Paused;

	CurrentTopUI = NAME_None;
	CurrentAllowedTransition.Empty();


	// -----------------------------------------------------
	// [추가] Objective Config
	// -----------------------------------------------------

	ObjectiveConfigTable = nullptr;

	// 현재 기능을 깨지 않도록, 실제 DataCore/AliveEnemyGate 연결 전에는
	// Kill 목표만 StageClear 필수 조건으로 사용한다.
	bEnforceCoreObjective = false;
	bEnforceRemainingEnemyObjective = false;

	CurrentCoresDestroyed = 0;
	CurrentRemainingEnemies = 0;
	bBossDefeated = false;


	// -----------------------------------------------------
	// [추가] Upgrade Grant Config
	// -----------------------------------------------------

	UpgradeGrantConfigTable = nullptr;
	ActiveUpgradeGrantId = NAME_None;
	PendingUpgradeSelections = 0;


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
// [추가] Tick - 숫자 1 / 2 / 3 무기 전환
// =========================================================

void ARGGameModeBase::Tick(
	float DeltaSeconds
)
{
	Super::Tick(
		DeltaSeconds
	);


	// -----------------------------------------------------
	// 기능 비활성화 상태면 아무것도 하지 않는다.
	// -----------------------------------------------------

	if (!bEnableNumberKeyWeaponSwitch)
	{
		return;
	}


	// -----------------------------------------------------
	// RunFlow가 Gameplay 입력을 허용할 때만 무기 교체 허용.
	//
	// Upgrade / Pause / Loading 등 UIOnly/Blocked 상태에서
	// 숫자키가 다른 UI 입력과 충돌하는 것을 방지한다.
	// -----------------------------------------------------

	if (
		CurrentInputPolicy !=
		ERGRunInputPolicy::Gameplay
		)
	{
		return;
	}


	APlayerController* PlayerController =
		GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;


	if (!PlayerController)
	{
		return;
	}


	// -----------------------------------------------------
	// Keyboard 1
	// -----------------------------------------------------

	if (
		PlayerController->WasInputKeyJustPressed(
			EKeys::One
		)
		)
	{
		SwitchPlayerWeaponSlot(
			1
		);

		return;
	}


	// -----------------------------------------------------
	// Keyboard 2
	// -----------------------------------------------------

	if (
		PlayerController->WasInputKeyJustPressed(
			EKeys::Two
		)
		)
	{
		SwitchPlayerWeaponSlot(
			2
		);

		return;
	}


	// -----------------------------------------------------
	// Keyboard 3
	// -----------------------------------------------------

	if (
		PlayerController->WasInputKeyJustPressed(
			EKeys::Three
		)
		)
	{
		SwitchPlayerWeaponSlot(
			3
		);

		return;
	}
}


// =========================================================
// [추가] SwitchPlayerWeaponSlot
// =========================================================

bool ARGGameModeBase::SwitchPlayerWeaponSlot(
	int32 SlotIndex
)
{
	TSubclassOf<ARGBaseWeapon> SelectedWeaponClass =
		nullptr;


	switch (SlotIndex)
	{
	case 1:
	{
		SelectedWeaponClass =
			RifleWeaponClass;

		break;
	}


	case 2:
	{
		SelectedWeaponClass =
			ShotgunWeaponClass;

		break;
	}


	case 3:
	{
		SelectedWeaponClass =
			RailgunWeaponClass;

		break;
	}


	default:
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"Invalid weapon slot: %d"
			),
			SlotIndex
		);

		return false;
	}
	}


	if (!SelectedWeaponClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"Weapon class is not assigned for slot %d."
			),
			SlotIndex
		);

		return false;
	}


	const bool bResult =
		EquipPlayerWeaponClass(
			SelectedWeaponClass
		);


	if (bResult)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[RGGameMode] "
				"Weapon Slot %d equipped."
			),
			SlotIndex
		);
	}


	return bResult;
}


// =========================================================
// [추가] EquipPlayerWeaponClass
// =========================================================

bool ARGGameModeBase::EquipPlayerWeaponClass(
	TSubclassOf<ARGBaseWeapon> NewWeaponClass
)
{
	if (!NewWeaponClass)
	{
		return false;
	}


	ARGCharacter* PlayerCharacter =
		Cast<ARGCharacter>(
			UGameplayStatics::GetPlayerCharacter(
				this,
				0
			)
		);


	if (!IsValid(PlayerCharacter))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"RGCharacter not found. "
				"Weapon switch failed."
			)
		);

		return false;
	}


	// -----------------------------------------------------
	// 같은 클래스의 무기를 이미 들고 있으면
	// 불필요하게 Destroy -> Spawn하지 않는다.
	// -----------------------------------------------------

	if (
		IsValid(
			PlayerCharacter->GetCurrentWeapon()
		) &&
		PlayerCharacter
		->GetCurrentWeapon()
		->GetClass() ==
		NewWeaponClass.Get()
		)
	{
		return true;
	}


	// -----------------------------------------------------
	// 핵심:
	// 새로운 교체 로직을 만들지 않고
	// 기존 Character::EquipWeapon()을 그대로 사용한다.
	//
	// 기존 EquipWeapon() 내부에서:
	// - 기존 무기 정리
	// - 새 무기 Spawn
	// - WeaponSocket 부착
	// - Animation Delegate 연결
	// - OnWeaponEquipped Broadcast
	//
	// 가 수행되므로 기존 UI/HUD 연동도 유지된다.
	// -----------------------------------------------------

	PlayerCharacter->EquipWeapon(
		NewWeaponClass
	);


	return
		IsValid(
			PlayerCharacter->GetCurrentWeapon()
		);
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

			// [추가] UpgradeGrant 완료 시점을 GameMode가 받을 수 있도록 연결
			Progression->OnCoreUpgradeApplied.RemoveDynamic(
				this,
				&ARGGameModeBase::HandleCoreUpgradeApplied
			);

			Progression->OnCoreUpgradeApplied.AddDynamic(
				this,
				&ARGGameModeBase::HandleCoreUpgradeApplied
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


	// [추가] Progression Delegate 연결 해제
	if (UGameInstance* GI = GetGameInstance())
	{
		if (
			URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>()
			)
		{
			Progression->OnCoreUpgradeApplied.RemoveDynamic(
				this,
				&ARGGameModeBase::HandleCoreUpgradeApplied
			);
		}
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


	// 현재 Stage Runtime Objective 초기화
	CurrentKills = 0;
	CurrentCoresDestroyed = 0;
	CurrentRemainingEnemies = 0;
	bBossDefeated = false;

	ActiveUpgradeGrantId = NAME_None;
	PendingUpgradeSelections = 0;


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

	// [추가] DT_ObjectiveConfig Kill_Default 진행도도 함께 알린다.
	BroadcastObjectiveProgress(
		TEXT("Kill_Default")
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
// [추가] GetRunFlowRowNameForState
// =========================================================

FName ARGGameModeBase::GetRunFlowRowNameForState(
	ERunState State
) const
{
	switch (State)
	{
	case ERunState::Init:
		return TEXT("WeaponSelect");

	case ERunState::Combat:
		return TEXT("Combat");

	case ERunState::Pause:
		return TEXT("InventoryPause");

	case ERunState::Upgrade:
		return TEXT("CoreUpgrade");

	case ERunState::RestHub:
		return TEXT("RestHub");

	case ERunState::Loading:
		return TEXT("Loading");

	case ERunState::Result:
		return TEXT("Result");

	default:
		return NAME_None;
	}
}


// =========================================================
// [추가] ApplyRunFlowConfigForState
// =========================================================

bool ARGGameModeBase::ApplyRunFlowConfigForState(
	ERunState State
)
{
	if (!IsValid(RunFlowConfigTable))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"RunFlowConfigTable is not assigned. "
				"Using legacy state policy."
			)
		);

		ApplyLegacyRunFlowFallback(
			State
		);

		return false;
	}


	if (
		RunFlowConfigTable->GetRowStruct() !=
		FRGRunFlowConfigRow::StaticStruct()
		)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"RunFlowConfigTable RowStruct mismatch. "
				"Expected FRGRunFlowConfigRow."
			)
		);

		ApplyLegacyRunFlowFallback(
			State
		);

		return false;
	}


	const FName RowName =
		GetRunFlowRowNameForState(
			State
		);


	if (RowName.IsNone())
	{
		ApplyLegacyRunFlowFallback(
			State
		);

		return false;
	}


	const FRGRunFlowConfigRow* FlowRow =
		RunFlowConfigTable
		->FindRow<FRGRunFlowConfigRow>(
			RowName,
			TEXT("ApplyRunFlowConfigForState"),
			false
		);


	if (!FlowRow)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"RunFlowConfig row not found: %s"
			),
			*RowName.ToString()
		);

		ApplyLegacyRunFlowFallback(
			State
		);

		return false;
	}


	// -----------------------------------------------------
	// DataTable 정책 저장
	// -----------------------------------------------------

	CurrentInputPolicy =
		FlowRow->InputPolicy;

	CurrentTimePolicy =
		FlowRow->TimePolicy;

	CurrentAIState =
		FlowRow->AIState;

	CurrentTopUI =
		FlowRow->TopUI;

	CurrentAllowedTransition =
		FlowRow->AllowedTransition;


	// -----------------------------------------------------
	// GameMode가 직접 담당하는 정책 적용
	// -----------------------------------------------------

	ApplyInputPolicy(
		CurrentInputPolicy
	);

	ApplyTimePolicy(
		CurrentTimePolicy
	);


	// -----------------------------------------------------
	// AI / UI 등 외부 시스템에는 Delegate로 전달
	// -----------------------------------------------------

	OnRunFlowPolicyChanged.Broadcast(
		CurrentInputPolicy,
		CurrentTimePolicy,
		CurrentAIState,
		CurrentTopUI
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] "
			"RunFlow applied. "
			"State=%d / Row=%s / "
			"Input=%d / Time=%d / AI=%d / "
			"TopUI=%s / Transition=%s"
		),
		static_cast<int32>(State),
		*RowName.ToString(),
		static_cast<int32>(CurrentInputPolicy),
		static_cast<int32>(CurrentTimePolicy),
		static_cast<int32>(CurrentAIState),
		*CurrentTopUI.ToString(),
		*CurrentAllowedTransition
	);


	return true;
}


// =========================================================
// [추가] ApplyLegacyRunFlowFallback
// =========================================================

void ARGGameModeBase::ApplyLegacyRunFlowFallback(
	ERunState State
)
{
	// -----------------------------------------------------
	// 기존 코드의 의미를 그대로 보존하는 fallback.
	// DataTable 연결 실패가 게임 진행 중단으로 이어지지 않게 한다.
	// -----------------------------------------------------

	switch (State)
	{
	case ERunState::Combat:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::Gameplay;

		CurrentTimePolicy =
			ERGRunTimePolicy::Countdown;

		CurrentAIState =
			ERGRunAIState::Active;

		CurrentTopUI =
			NAME_None;

		break;
	}


	case ERunState::RestHub:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::Gameplay;

		CurrentTimePolicy =
			ERGRunTimePolicy::NoObjective;

		CurrentAIState =
			ERGRunAIState::Disabled;

		CurrentTopUI =
			NAME_None;

		break;
	}


	case ERunState::Pause:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::UIOnly;

		CurrentTimePolicy =
			ERGRunTimePolicy::Paused;

		CurrentAIState =
			ERGRunAIState::Paused;

		CurrentTopUI =
			TEXT("Inventory");

		break;
	}


	case ERunState::Upgrade:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::UIOnly;

		CurrentTimePolicy =
			ERGRunTimePolicy::Paused;

		CurrentAIState =
			ERGRunAIState::Paused;

		CurrentTopUI =
			TEXT("UpgradeSelection");

		break;
	}


	case ERunState::Loading:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::Blocked;

		CurrentTimePolicy =
			ERGRunTimePolicy::Paused;

		CurrentAIState =
			ERGRunAIState::Paused;

		CurrentTopUI =
			TEXT("Transition");

		break;
	}


	case ERunState::Result:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::UIOnly;

		CurrentTimePolicy =
			ERGRunTimePolicy::Paused;

		CurrentAIState =
			ERGRunAIState::Paused;

		CurrentTopUI =
			TEXT("Result");

		break;
	}


	case ERunState::Init:
	default:
	{
		CurrentInputPolicy =
			ERGRunInputPolicy::UIOnly;

		CurrentTimePolicy =
			ERGRunTimePolicy::Paused;

		CurrentAIState =
			ERGRunAIState::Paused;

		CurrentTopUI =
			TEXT("WeaponSelection");

		break;
	}
	}


	CurrentAllowedTransition.Empty();


	ApplyInputPolicy(
		CurrentInputPolicy
	);

	ApplyTimePolicy(
		CurrentTimePolicy
	);


	OnRunFlowPolicyChanged.Broadcast(
		CurrentInputPolicy,
		CurrentTimePolicy,
		CurrentAIState,
		CurrentTopUI
	);
}


// =========================================================
// [추가] ApplyInputPolicy
// =========================================================

void ARGGameModeBase::ApplyInputPolicy(
	ERGRunInputPolicy InputPolicy
)
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


	switch (InputPolicy)
	{
	case ERGRunInputPolicy::Gameplay:
	{
		// 이전 Pause / Loading 상태에서 걸어 둔 차단 해제
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);

		PlayerController->bShowMouseCursor =
			false;


		FInputModeGameOnly InputMode;

		PlayerController->SetInputMode(
			InputMode
		);

		break;
	}


	case ERGRunInputPolicy::UIOnly:
	{
		// UIOnly는 게임 입력 대신 Widget 입력을 우선한다.
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);

		PlayerController->bShowMouseCursor =
			true;


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


	case ERGRunInputPolicy::Blocked:
	{
		// Loading처럼 플레이어 조작을 받지 않는 상태.
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);

		PlayerController->bShowMouseCursor =
			false;


		FInputModeUIOnly InputMode;

		PlayerController->SetInputMode(
			InputMode
		);

		break;
	}


	default:
		break;
	}
}


// =========================================================
// [추가] ApplyTimePolicy
// =========================================================

void ARGGameModeBase::ApplyTimePolicy(
	ERGRunTimePolicy TimePolicy
)
{
	if (!GetWorld())
	{
		return;
	}


	FTimerManager& TimerManager =
		GetWorld()->GetTimerManager();


	switch (TimePolicy)
	{
	case ERGRunTimePolicy::Countdown:
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

		break;
	}


	case ERGRunTimePolicy::Paused:
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

		break;
	}


	case ERGRunTimePolicy::NoObjective:
	{
		// RestHub처럼 목표 시간이 없는 상태는
		// 이전 Combat Timer 자체를 정리한다.
		TimerManager.ClearTimer(
			RunTimerHandle
		);

		break;
	}


	default:
		break;
	}
}



// =========================================================
// [추가] Objective Config Helpers
// =========================================================

const FRGObjectiveConfigRow* ARGGameModeBase::FindObjectiveConfigRow(
	FName ObjectiveId
) const
{
	if (
		ObjectiveId.IsNone() ||
		!IsValid(ObjectiveConfigTable)
		)
	{
		return nullptr;
	}

	if (
		ObjectiveConfigTable->GetRowStruct() !=
		FRGObjectiveConfigRow::StaticStruct()
		)
	{
		return nullptr;
	}

	return ObjectiveConfigTable->FindRow<FRGObjectiveConfigRow>(
		ObjectiveId,
		TEXT("FindObjectiveConfigRow"),
		false
	);
}


int32 ARGGameModeBase::ResolveObjectiveRequiredValue(
	FName ObjectiveId,
	const FRGObjectiveConfigRow& Row
) const
{
	if (!Row.bUseStageConfigValue)
	{
		return FMath::Max(0, Row.RequiredValue);
	}

	switch (Row.ObjectiveType)
	{
	case ERGObjectiveType::KillCount:
		return FMath::Max(0, TargetKillsToClear);

	case ERGObjectiveType::DataCoreDestroyed:
		return FMath::Max(0, RequiredCoresToClear);

	default:
		return FMath::Max(0, Row.RequiredValue);
	}
}


void ARGGameModeBase::BroadcastObjectiveProgress(
	FName ObjectiveId
)
{
	const FRGObjectiveConfigRow* Row =
		FindObjectiveConfigRow(ObjectiveId);

	if (!Row)
	{
		return;
	}

	const int32 RequiredValue =
		ResolveObjectiveRequiredValue(
			ObjectiveId,
			*Row
		);

	int32 CurrentValue = 0;
	bool bCompleted = false;

	switch (Row->ObjectiveType)
	{
	case ERGObjectiveType::KillCount:
		CurrentValue = CurrentKills;
		bCompleted =
			RequiredValue <= 0 ||
			CurrentKills >= RequiredValue;
		break;

	case ERGObjectiveType::DataCoreDestroyed:
		CurrentValue = CurrentCoresDestroyed;
		bCompleted =
			RequiredValue <= 0 ||
			CurrentCoresDestroyed >= RequiredValue;
		break;

	case ERGObjectiveType::RemainingEnemy:
		CurrentValue = CurrentRemainingEnemies;
		bCompleted =
			CurrentRemainingEnemies <= RequiredValue;
		break;

	case ERGObjectiveType::BossKilled:
		CurrentValue = bBossDefeated ? 1 : 0;
		bCompleted = bBossDefeated;
		break;

	default:
		break;
	}

	OnObjectiveProgressChanged.Broadcast(
		ObjectiveId,
		CurrentValue,
		RequiredValue,
		bCompleted
	);
}


// =========================================================
// [추가] DataCore Objective
// =========================================================

void ARGGameModeBase::OnDataCoreDestroyed()
{
	if (bIsRunEnded)
	{
		return;
	}

	++CurrentCoresDestroyed;

	AddScoreEvent(
		TEXT("DataCoreDestroyed")
	);

	BroadcastObjectiveProgress(
		TEXT("Core_Default")
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[RGGameMode] Core Objective: %d / %d"
		),
		CurrentCoresDestroyed,
		RequiredCoresToClear
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
// [추가] Remaining Enemy Objective
// =========================================================

void ARGGameModeBase::SetRemainingEnemyCount(
	int32 NewRemainingEnemyCount
)
{
	CurrentRemainingEnemies =
		FMath::Max(
			0,
			NewRemainingEnemyCount
		);

	BroadcastObjectiveProgress(
		TEXT("AliveEnemyGate")
	);

	if (
		!bIsRunEnded &&
		CheckStageClearCondition()
		)
	{
		CheckEndCondition(
			false,
			false
		);
	}
}


// =========================================================
// [추가] Boss Objective
// =========================================================

void ARGGameModeBase::OnBossKilled()
{
	if (bIsRunEnded)
	{
		return;
	}

	bBossDefeated = true;

	AddScoreEvent(
		TEXT("BossKilled")
	);

	BroadcastObjectiveProgress(
		TEXT("BossDefeat")
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RGGameMode] Boss objective completed.")
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
// [추가] Upgrade Grant Lookup
// =========================================================

const FRGUpgradeGrantConfigRow*
ARGGameModeBase::FindUpgradeGrantForCurrentStage(
	FName& OutGrantId
) const
{
	OutGrantId = NAME_None;

	if (
		CurrentStageConfigRowName.IsNone() ||
		!IsValid(UpgradeGrantConfigTable)
		)
	{
		return nullptr;
	}

	if (
		UpgradeGrantConfigTable->GetRowStruct() !=
		FRGUpgradeGrantConfigRow::StaticStruct()
		)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"UpgradeGrantConfigTable RowStruct mismatch."
			)
		);

		return nullptr;
	}

	const FString StageToken =
		CurrentStageConfigRowName.ToString();

	for (
		const FName& RowName :
		UpgradeGrantConfigTable->GetRowNames()
		)
	{
		const FRGUpgradeGrantConfigRow* Row =
			UpgradeGrantConfigTable
			->FindRow<FRGUpgradeGrantConfigRow>(
				RowName,
				TEXT("FindUpgradeGrantForCurrentStage"),
				false
			);

		if (!Row)
		{
			continue;
		}

		// 현재 DT는 TriggerCondition이 설명 문자열이므로,
		// 안정적인 부분인 Stage RowName 포함 여부만 사용한다.
		if (
			Row->TriggerCondition.Contains(
				StageToken,
				ESearchCase::IgnoreCase
			)
			)
		{
			OutGrantId = RowName;
			return Row;
		}
	}

	return nullptr;
}


// =========================================================
// [추가] Queue Upgrade Grant For RestHub
// =========================================================

bool ARGGameModeBase::QueueConfiguredUpgradeGrant()
{
	FName GrantId = NAME_None;

	const FRGUpgradeGrantConfigRow* GrantRow =
		FindUpgradeGrantForCurrentStage(
			GrantId
		);


	// 현재 Stage에 지급 설정이 없으면 정상적으로 false.
	if (!GrantRow)
	{
		return false;
	}


	URGProgressionSubsystem* Progression =
		GetGameInstance()
		? GetGameInstance()
		->GetSubsystem<URGProgressionSubsystem>()
		: nullptr;


	if (!Progression)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGGameMode] "
				"Cannot queue UpgradeGrant: "
				"RGProgressionSubsystem not found."
			)
		);

		return false;
	}


	const int32 CandidateCount =
		FMath::Max(
			1,
			GrantRow->CandidateCount
		);


	const int32 SelectionCount =
		FMath::Max(
			1,
			GrantRow->SelectionCount
		);


	// 현재 UI는 1회 선택 구조.
	// 데이터가 달라져도 즉시 크래시하지 않고 경고만 남긴다.
	if (SelectionCount != 1)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"UpgradeGrant '%s' SelectionCount=%d. "
				"Current CoreUpgrade UI supports one selection."
			),
			*GrantId.ToString(),
			SelectionCount
		);
	}


	// -----------------------------------------------------
	// 핵심:
	// StageClear 맵에서는 UI를 띄우지 않고 예약만 저장한다.
	// -----------------------------------------------------

	const bool bQueued =
		Progression->QueueCoreUpgradeChoice(
			GrantId,
			CandidateCount
		);


	if (bQueued)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"UpgradeGrant queued for TransitHub. "
				"Grant=%s / Candidates=%d / Result=%s"
			),
			*GrantId.ToString(),
			CandidateCount,
			*GrantRow->CompletionResult
		);
	}


	return bQueued;
}


// =========================================================
// [추가] Start Upgrade Grant
// =========================================================

bool ARGGameModeBase::TryStartConfiguredUpgradeGrant()
{
	FName GrantId = NAME_None;

	const FRGUpgradeGrantConfigRow* GrantRow =
		FindUpgradeGrantForCurrentStage(
			GrantId
		);

	if (!GrantRow)
	{
		return false;
	}

	URGProgressionSubsystem* Progression =
		GetGameInstance()
		? GetGameInstance()
		->GetSubsystem<URGProgressionSubsystem>()
		: nullptr;

	if (!Progression)
	{
		return false;
	}

	const int32 CandidateCount =
		FMath::Max(
			1,
			GrantRow->CandidateCount
		);

	const int32 SelectionCount =
		FMath::Max(
			1,
			GrantRow->SelectionCount
		);

	// 현재 Progression 핵심 강화 UI는 1회 선택 단위로 동작한다.
	// 현재 DataTable의 Evolution01/02도 SelectionCount=1이므로 그대로 일치한다.
	if (SelectionCount != 1)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGGameMode] "
				"UpgradeGrant '%s' SelectionCount=%d. "
				"Current CoreUpgrade UI supports one selection per grant."
			),
			*GrantId.ToString(),
			SelectionCount
		);
	}

	if (GrantRow->bPauseTime)
	{
		ChangeRunState(
			ERunState::Upgrade
		);
	}

	if (
		!Progression->PresentCoreUpgradeChoiceWithCount(
			CandidateCount
		)
		)
	{
		return false;
	}

	ActiveUpgradeGrantId = GrantId;
	PendingUpgradeSelections = 1;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGGameMode] "
			"UpgradeGrant started. "
			"Grant=%s / Candidates=%d / Result=%s"
		),
		*GrantId.ToString(),
		CandidateCount,
		*GrantRow->CompletionResult
	);

	return true;
}


// =========================================================
// [추가] Upgrade Grant Completion
// =========================================================

void ARGGameModeBase::HandleCoreUpgradeApplied(
	FName UpgradeId,
	int32 ActiveCoreUpgradeCount
)
{
	if (ActiveUpgradeGrantId.IsNone())
	{
		return;
	}

	PendingUpgradeSelections =
		FMath::Max(
			0,
			PendingUpgradeSelections - 1
		);

	if (PendingUpgradeSelections > 0)
	{
		return;
	}

	const FName CompletedGrant =
		ActiveUpgradeGrantId;

	ActiveUpgradeGrantId =
		NAME_None;

	OnUpgradeGrantCompleted.Broadcast(
		CompletedGrant
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGGameMode] "
			"UpgradeGrant completed. "
			"Grant=%s / Applied=%s / CoreCount=%d"
		),
		*CompletedGrant.ToString(),
		*UpgradeId.ToString(),
		ActiveCoreUpgradeCount
	);

	// 최종 Result가 아니라면 강화 선택 후 다시 자유 이동 상태로 복귀.
	if (
		StageCompletionDestination !=
		TEXT("Result")
		)
	{
		ChangeRunState(
			ERunState::RestHub
		);
	}
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


	// -----------------------------------------------------
	// [수정] 상태별 입력 / 시간 / AI / UI 정책을
	// DT_RunFlowConfig에서 읽어 적용한다.
	// -----------------------------------------------------

	ApplyRunFlowConfigForState(
		CurrentState
	);
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


	// [수정]
	// 특정 State 이름에 하드코딩하지 않고
	// DT_RunFlowConfig의 TimePolicy를 기준으로 Countdown 여부를 판단한다.
	if (
		CurrentTimePolicy !=
		ERGRunTimePolicy::Countdown
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
	// -----------------------------------------------------
	// Boss Stage:
	// RequiredKills/RequiredCores가 모두 0이고 Destination=Result이면
	// DT_ObjectiveConfig의 BossDefeat을 완료 조건으로 사용한다.
	// -----------------------------------------------------

	const bool bBossStage =
		TargetKillsToClear <= 0 &&
		RequiredCoresToClear <= 0 &&
		StageCompletionDestination == TEXT("Result");

	if (bBossStage)
	{
		return bBossDefeated;
	}


	// -----------------------------------------------------
	// 일반 Stage: Kill objective
	// -----------------------------------------------------

	if (
		TargetKillsToClear > 0 &&
		CurrentKills < TargetKillsToClear
		)
	{
		return false;
	}


	// -----------------------------------------------------
	// Core objective
	//
	// 현재 실제 DataCore 이벤트가 없는 맵을 깨지 않도록
	// bEnforceCoreObjective=true일 때만 StageClear 필수 조건으로 사용.
	// -----------------------------------------------------

	if (
		bEnforceCoreObjective &&
		RequiredCoresToClear > 0 &&
		CurrentCoresDestroyed < RequiredCoresToClear
		)
	{
		return false;
	}


	// -----------------------------------------------------
	// RemainingEnemy objective
	//
	// 기존 요구사항 "목표 킬 수 달성 -> 포탈 활성화"를 유지하기 위해
	// 기본값은 비활성(false).
	// -----------------------------------------------------

	if (bEnforceRemainingEnemyObjective)
	{
		const FRGObjectiveConfigRow* AliveRow =
			FindObjectiveConfigRow(
				TEXT("AliveEnemyGate")
			);

		const int32 RequiredRemaining =
			AliveRow
			? ResolveObjectiveRequiredValue(
				TEXT("AliveEnemyGate"),
				*AliveRow
			)
			: 0;

		if (
			CurrentRemainingEnemies >
			RequiredRemaining
			)
		{
			return false;
		}
	}


	// Kill 목표가 0인 일반 스테이지가 실수로 자동 완료되지 않도록 한다.
	return TargetKillsToClear > 0;
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
	// 최종 Boss Result
	// -----------------------------------------------------

	if (
		StageCompletionDestination ==
		TEXT("Result")
		)
	{
		ChangeRunState(
			ERunState::Result
		);

		OnStageCleared.Broadcast();

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[STAGE CLEAR] Final result stage completed."
			)
		);

		return;
	}


	// -----------------------------------------------------
	// [수정] DT_UpgradeGrantConfig 확인
	// -----------------------------------------------------
	//
	// 기존:
	// StageClear 즉시 핵심 강화 UI 표시
	//
	// 변경:
	// StageClear에서는 GameInstanceSubsystem에 "예약"만 저장.
	// 실제 UI는 L_Map_TransitHUB에 도착한 뒤
	// BP_CombatUIManager가 ConsumePendingCoreUpgradeChoice()를 호출할 때 표시.
	// -----------------------------------------------------

	const bool bUpgradeGrantQueued =
		QueueConfiguredUpgradeGrant();


	// -----------------------------------------------------
	// 전투맵에서는 StageClear 후 포탈을 사용할 수 있도록
	// RestHub 정책 상태로 전환한다.
	//
	// 핵심 강화 UI는 여기서 띄우지 않는다.
	// -----------------------------------------------------

	ChangeRunState(
		ERunState::RestHub
	);


	// -----------------------------------------------------
	// Portal/외부 시스템 Stage Clear 알림
	// -----------------------------------------------------

	OnStageCleared.Broadcast();


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[STAGE CLEAR] "
			"Stage=%s / Destination=%s / UpgradeGrantQueued=%s"
		),
		*CurrentStageConfigRowName.ToString(),
		*StageCompletionDestination.ToString(),
		bUpgradeGrantQueued ? TEXT("YES") : TEXT("NO")
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