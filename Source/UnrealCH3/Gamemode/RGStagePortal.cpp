// Fill out your copyright notice in the Description page of Project Settings.

#include "Gamemode/RGStagePortal.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Gamemode/DataTableStruct/RGRunConfigRows.h"
#include "Gamemode/RGGameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


// =========================================================
// Constructor
// =========================================================

ARGStagePortal::ARGStagePortal()
{
	PrimaryActorTick.bCanEverTick = false;

	// -----------------------------------------------------
	// 기본 상태
	// -----------------------------------------------------

	bStartActive = false;

	bPortalActive = false;
	bTravelStarted = false;
	bPortalConfigApplied = false;

	NextLevelName = NAME_None;

	// -----------------------------------------------------
	// [추가] PortalConfig 기본값
	// -----------------------------------------------------

	PortalConfigTable = nullptr;

	// 전투 스테이지 포탈을 기본값으로 둔다.
	PortalConfigRowName = TEXT("CombatExit");

	ConfiguredDestination = NAME_None;
	VisibilityCondition = NAME_None;

	TransitionDelaySeconds = 0.0f;

	CachedInitialState =
		static_cast<uint8>(
			ERGPortalInitialState::Hidden
			);

	CachedInteractionType =
		static_cast<uint8>(
			ERGPortalInteractionType::EnterOrUse
			);


	// -----------------------------------------------------
	// Root
	// -----------------------------------------------------

	PortalRoot =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("PortalRoot")
		);

	SetRootComponent(
		PortalRoot
	);


	// -----------------------------------------------------
	// Visual Mesh
	// -----------------------------------------------------

	PortalMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("PortalMesh")
		);

	PortalMesh->SetupAttachment(
		PortalRoot
	);

	PortalMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);


	// -----------------------------------------------------
	// Trigger
	// -----------------------------------------------------

	PortalTrigger =
		CreateDefaultSubobject<UBoxComponent>(
			TEXT("PortalTrigger")
		);

	PortalTrigger->SetupAttachment(
		PortalRoot
	);

	PortalTrigger->SetBoxExtent(
		FVector(
			100.0f,
			100.0f,
			150.0f
		)
	);

	PortalTrigger->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly
	);

	PortalTrigger->SetCollisionObjectType(
		ECC_WorldDynamic
	);

	PortalTrigger->SetCollisionResponseToAllChannels(
		ECR_Ignore
	);

	PortalTrigger->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Overlap
	);
}


// =========================================================
// BeginPlay
// =========================================================

void ARGStagePortal::BeginPlay()
{
	Super::BeginPlay();


	// -----------------------------------------------------
	// Trigger 이벤트 연결
	// -----------------------------------------------------

	if (IsValid(PortalTrigger))
	{
		PortalTrigger->OnComponentBeginOverlap.AddUniqueDynamic(
			this,
			&ARGStagePortal::HandlePortalOverlap
		);
	}


	// -----------------------------------------------------
	// [추가] DT_PortalConfig 적용
	// -----------------------------------------------------

	bPortalConfigApplied =
		ApplyPortalConfig();


	// -----------------------------------------------------
	// 기존 GameMode Stage Clear Delegate 연결
	// -----------------------------------------------------

	if (
		ARGGameModeBase* GameMode =
		Cast<ARGGameModeBase>(
			UGameplayStatics::GetGameMode(this)
		)
		)
	{
		BoundGameMode =
			GameMode;


		GameMode->OnStageCleared.AddUniqueDynamic(
			this,
			&ARGStagePortal::HandleStageCleared
		);


		// -------------------------------------------------
		// Portal이 늦게 생성되었는데
		// StageClear가 이미 완료된 경우 보정
		// -------------------------------------------------

		if (
			VisibilityCondition == TEXT("StageComplete") &&
			GameMode->CheckStageClearCondition() &&
			DoesStageDestinationMatch()
			)
		{
			ActivatePortal();

			return;
		}
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGStagePortal] "
				"RGGameModeBase not found. "
				"Portal=%s"
			),
			*GetName()
		);
	}


	// -----------------------------------------------------
	// [추가] 초기 상태 적용
	// -----------------------------------------------------

	if (bPortalConfigApplied)
	{
		const ERGPortalInitialState InitialState =
			static_cast<ERGPortalInitialState>(
				CachedInitialState
				);


		switch (InitialState)
		{
		case ERGPortalInitialState::Hidden:
		{
			DeactivatePortal();

			break;
		}


		case ERGPortalInitialState::VisibleLocked:
		{
			SetPortalVisibleLocked();

			break;
		}


		default:
		{
			DeactivatePortal();

			break;
		}
		}
	}
	else
	{
		// ---------------------------------------------
		// DataTable 미지정 시 기존 동작 유지
		// ---------------------------------------------

		if (bStartActive)
		{
			ActivatePortal();
		}
		else
		{
			DeactivatePortal();
		}
	}
}


// =========================================================
// EndPlay
// =========================================================

void ARGStagePortal::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	// -----------------------------------------------------
	// [추가] 이동 지연 타이머 정리
	// -----------------------------------------------------

	if (GetWorld())
	{
		GetWorld()
			->GetTimerManager()
			.ClearTimer(
				TransitionTimerHandle
			);
	}


	// -----------------------------------------------------
	// Delegate 정리
	// -----------------------------------------------------

	if (BoundGameMode.IsValid())
	{
		BoundGameMode
			->OnStageCleared
			.RemoveDynamic(
				this,
				&ARGStagePortal::HandleStageCleared
			);
	}


	if (IsValid(PortalTrigger))
	{
		PortalTrigger
			->OnComponentBeginOverlap
			.RemoveDynamic(
				this,
				&ARGStagePortal::HandlePortalOverlap
			);
	}


	Super::EndPlay(
		EndPlayReason
	);
}


// =========================================================
// [추가] ApplyPortalConfig
// =========================================================

bool ARGStagePortal::ApplyPortalConfig()
{
	if (!IsValid(PortalConfigTable))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGStagePortal] "
				"PortalConfigTable is not assigned. "
				"Legacy portal settings will be used."
			)
		);

		return false;
	}


	if (
		PortalConfigTable->GetRowStruct() !=
		FRGPortalConfigRow::StaticStruct()
		)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGStagePortal] "
				"PortalConfigTable RowStruct mismatch. "
				"Expected FRGPortalConfigRow."
			)
		);

		return false;
	}


	if (PortalConfigRowName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGStagePortal] "
				"PortalConfigRowName is None."
			)
		);

		return false;
	}


	const FRGPortalConfigRow* PortalRow =
		PortalConfigTable
		->FindRow<FRGPortalConfigRow>(
			PortalConfigRowName,
			TEXT("ApplyPortalConfig"),
			false
		);


	if (!PortalRow)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGStagePortal] "
				"PortalConfig row not found: %s"
			),
			*PortalConfigRowName.ToString()
		);

		return false;
	}


	CachedInitialState =
		static_cast<uint8>(
			PortalRow->InitialState
			);

	CachedInteractionType =
		static_cast<uint8>(
			PortalRow->InteractionType
			);

	VisibilityCondition =
		PortalRow->VisibilityCondition;

	TransitionDelaySeconds =
		FMath::Max(
			0.0f,
			PortalRow->TransitionDelaySeconds
		);

	ConfiguredDestination =
		PortalRow->Destination;


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGStagePortal] "
			"PortalConfig applied. "
			"Row=%s / Condition=%s / "
			"Delay=%.2f / Destination=%s / "
			"NextLevel=%s"
		),
		*PortalConfigRowName.ToString(),
		*VisibilityCondition.ToString(),
		TransitionDelaySeconds,
		*ConfiguredDestination.ToString(),
		*NextLevelName.ToString()
	);


	return true;
}


// =========================================================
// HandleStageCleared
// =========================================================

void ARGStagePortal::HandleStageCleared()
{
	// -----------------------------------------------------
	// [수정]
	// StageComplete 조건의 포탈만 StageClear에서 활성화.
	//
	// RestHubReady / CoreUpgrade2Applied 같은 포탈은
	// 해당 시스템 연결 단계에서 별도로 ActivatePortal() 한다.
	// -----------------------------------------------------

	if (
		bPortalConfigApplied &&
		VisibilityCondition != TEXT("StageComplete")
		)
	{
		return;
	}


	// -----------------------------------------------------
	// [추가]
	// StageConfig의 CompletionDestination과
	// PortalConfig의 Destination이 다르면 이 포탈은
	// 현재 Stage의 출구가 아니므로 활성화하지 않는다.
	// -----------------------------------------------------

	if (!DoesStageDestinationMatch())
	{
		return;
	}


	ActivatePortal();
}


// =========================================================
// [추가] DoesStageDestinationMatch
// =========================================================

bool ARGStagePortal::DoesStageDestinationMatch() const
{
	// -----------------------------------------------------
	// DataTable 미사용 시 기존 동작 보장
	// -----------------------------------------------------

	if (!bPortalConfigApplied)
	{
		return true;
	}


	// -----------------------------------------------------
	// 목적지가 없는 Config라면 비교하지 않는다.
	// -----------------------------------------------------

	if (ConfiguredDestination.IsNone())
	{
		return true;
	}


	if (!BoundGameMode.IsValid())
	{
		return true;
	}


	const FName StageDestination =
		BoundGameMode
		->GetStageCompletionDestination();


	// StageConfig 쪽 값이 아직 없으면
	// PortalConfig만으로 동작하도록 허용한다.
	if (StageDestination.IsNone())
	{
		return true;
	}


	const bool bMatches =
		StageDestination ==
		ConfiguredDestination;


	if (!bMatches)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[RGStagePortal] "
				"Portal ignored for this stage. "
				"StageDestination=%s / "
				"PortalDestination=%s / Portal=%s"
			),
			*StageDestination.ToString(),
			*ConfiguredDestination.ToString(),
			*GetName()
		);
	}


	return bMatches;
}


// =========================================================
// ActivatePortal
// =========================================================

void ARGStagePortal::ActivatePortal()
{
	if (bPortalActive)
	{
		return;
	}


	bPortalActive = true;
	bTravelStarted = false;


	// -----------------------------------------------------
	// 화면에 포탈 표시
	// -----------------------------------------------------

	if (IsValid(PortalMesh))
	{
		PortalMesh->SetVisibility(
			true,
			true
		);
	}


	// -----------------------------------------------------
	// 플레이어 진입 허용
	// -----------------------------------------------------

	if (IsValid(PortalTrigger))
	{
		PortalTrigger->SetCollisionEnabled(
			ECollisionEnabled::QueryOnly
		);
	}


	OnPortalActivated();


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGStagePortal] "
			"Portal ACTIVATED. "
			"Portal=%s / Config=%s / "
			"Destination=%s / NextLevel=%s"
		),
		*GetName(),
		*PortalConfigRowName.ToString(),
		*ConfiguredDestination.ToString(),
		*NextLevelName.ToString()
	);
}


// =========================================================
// DeactivatePortal
// =========================================================

void ARGStagePortal::DeactivatePortal()
{
	bPortalActive = false;
	bTravelStarted = false;


	if (IsValid(PortalMesh))
	{
		PortalMesh->SetVisibility(
			false,
			true
		);
	}


	if (IsValid(PortalTrigger))
	{
		PortalTrigger->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}


	OnPortalDeactivated();
}


// =========================================================
// [추가] SetPortalVisibleLocked
// =========================================================

void ARGStagePortal::SetPortalVisibleLocked()
{
	bPortalActive = false;
	bTravelStarted = false;


	// -----------------------------------------------------
	// 보이지만 사용 불가
	// -----------------------------------------------------

	if (IsValid(PortalMesh))
	{
		PortalMesh->SetVisibility(
			true,
			true
		);
	}


	if (IsValid(PortalTrigger))
	{
		PortalTrigger->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}


	OnPortalLockedVisible();
}


// =========================================================
// HandlePortalOverlap
// =========================================================

void ARGStagePortal::HandlePortalOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	// -----------------------------------------------------
	// 꺼져 있거나 이미 이동 시작했으면 무시
	// -----------------------------------------------------

	if (
		!bPortalActive ||
		bTravelStarted
		)
	{
		return;
	}


	// -----------------------------------------------------
	// 플레이어 Pawn만 사용 가능
	// -----------------------------------------------------

	APawn* Pawn =
		Cast<APawn>(
			OtherActor
		);


	if (
		!IsValid(Pawn) ||
		!Pawn->IsPlayerControlled()
		)
	{
		return;
	}


	// -----------------------------------------------------
	// [추가]
	// 현재 DT_PortalConfig의 EnterOnly / EnterOrUse는
	// 둘 다 "진입"을 허용하므로 Overlap 경로는 공통 사용.
	//
	// 추후 E키 전용 상호작용이 필요하면
	// EnterOrUse에만 Use 입력 경로를 추가하면 된다.
	// -----------------------------------------------------

	const ERGPortalInteractionType InteractionType =
		static_cast<ERGPortalInteractionType>(
			CachedInteractionType
			);

	if (
		InteractionType !=
		ERGPortalInteractionType::EnterOnly &&
		InteractionType !=
		ERGPortalInteractionType::EnterOrUse
		)
	{
		return;
	}


	// -----------------------------------------------------
	// 실제 Level 이름 확인
	// -----------------------------------------------------

	if (NextLevelName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGStagePortal] "
				"NextLevelName is empty. "
				"Config Destination=%s / Portal=%s"
			),
			*ConfiguredDestination.ToString(),
			*GetName()
		);

		return;
	}


	// -----------------------------------------------------
	// 중복 진입 차단
	// -----------------------------------------------------

	bTravelStarted = true;


	if (IsValid(PortalTrigger))
	{
		PortalTrigger->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}


	// -----------------------------------------------------
	// RunState -> Loading
	// -----------------------------------------------------

	if (
		ARGGameModeBase* GameMode =
		Cast<ARGGameModeBase>(
			UGameplayStatics::GetGameMode(this)
		)
		)
	{
		GameMode->ChangeRunState(
			ERunState::Loading
		);
	}


	// -----------------------------------------------------
	// [추가] DataTable의 TransitionDelaySeconds 적용
	// -----------------------------------------------------

	if (
		TransitionDelaySeconds <= 0.0f ||
		!GetWorld()
		)
	{
		PerformTravel();

		return;
	}


	GetWorld()
		->GetTimerManager()
		.SetTimer(
			TransitionTimerHandle,
			this,
			&ARGStagePortal::PerformTravel,
			TransitionDelaySeconds,
			false
		);


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGStagePortal] "
			"Travel scheduled. "
			"Delay=%.2f / %s -> %s"
		),
		TransitionDelaySeconds,
		*UGameplayStatics::GetCurrentLevelName(
			this,
			true
		),
		*NextLevelName.ToString()
	);
}


// =========================================================
// [추가] PerformTravel
// =========================================================

void ARGStagePortal::PerformTravel()
{
	if (NextLevelName.IsNone())
	{
		bTravelStarted = false;

		return;
	}


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[RGStagePortal] "
			"Travel: %s -> %s "
			"(LogicalDestination=%s)"
		),
		*UGameplayStatics::GetCurrentLevelName(
			this,
			true
		),
		*NextLevelName.ToString(),
		*ConfiguredDestination.ToString()
	);


	UGameplayStatics::OpenLevel(
		this,
		NextLevelName
	);
}
