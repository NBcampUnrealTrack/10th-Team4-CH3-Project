// Fill out your copyright notice in the Description page of Project Settings.

#include "Gamemode/RGStagePortal.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Gamemode/RGGameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"


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
	NextLevelName = NAME_None;

	// -----------------------------------------------------
	// Root
	// -----------------------------------------------------
	PortalRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PortalRoot"));
	SetRootComponent(PortalRoot);

	// -----------------------------------------------------
	// Visual Mesh
	// -----------------------------------------------------
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(PortalRoot);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// -----------------------------------------------------
	// Trigger
	// -----------------------------------------------------
	PortalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalTrigger"));
	PortalTrigger->SetupAttachment(PortalRoot);
	PortalTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 150.0f));
	PortalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PortalTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}


// =========================================================
// BeginPlay
// =========================================================

void ARGStagePortal::BeginPlay()
{
	Super::BeginPlay();

	// [추가] Trigger 이벤트 연결
	PortalTrigger->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&ARGStagePortal::HandlePortalOverlap
	);

	// [추가] 기존 GameMode의 Stage Clear Delegate에 연결
	if (ARGGameModeBase* GameMode = Cast<ARGGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		BoundGameMode = GameMode;

		GameMode->OnStageCleared.AddUniqueDynamic(
			this,
			&ARGStagePortal::HandleStageCleared
		);

		// 혹시 Portal이 늦게 생성된 경우에도 이미 Clear 조건을 만족했다면 활성화
		if (GameMode->CheckStageClearCondition())
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
			TEXT("[RGStagePortal] RGGameModeBase not found. Portal=%s"),
			*GetName()
		);
	}

	// [추가] 일반 전투 맵에서는 기본적으로 꺼진 상태로 시작
	if (bStartActive)
	{
		ActivatePortal();
	}
	else
	{
		DeactivatePortal();
	}
}


// =========================================================
// EndPlay
// =========================================================

void ARGStagePortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// [추가] Delegate 정리
	if (BoundGameMode.IsValid())
	{
		BoundGameMode->OnStageCleared.RemoveDynamic(
			this,
			&ARGStagePortal::HandleStageCleared
		);
	}

	if (IsValid(PortalTrigger))
	{
		PortalTrigger->OnComponentBeginOverlap.RemoveDynamic(
			this,
			&ARGStagePortal::HandlePortalOverlap
		);
	}

	Super::EndPlay(EndPlayReason);
}


// =========================================================
// HandleStageCleared
// =========================================================

void ARGStagePortal::HandleStageCleared()
{
	// [추가] TargetKillsToClear 충족 -> GameMode Stage Clear -> Portal 활성화
	ActivatePortal();
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

	// [추가] 화면에 포탈 표시
	if (IsValid(PortalMesh))
	{
		PortalMesh->SetVisibility(true, true);
	}

	// [추가] 플레이어 진입 허용
	if (IsValid(PortalTrigger))
	{
		PortalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// Blueprint에서 원하는 활성화 연출을 추가할 수 있음
	OnPortalActivated();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RGStagePortal] Portal ACTIVATED. Portal=%s / NextLevel=%s"),
		*GetName(),
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

	// [추가] Stage Clear 전에는 포탈을 숨긴다.
	if (IsValid(PortalMesh))
	{
		PortalMesh->SetVisibility(false, true);
	}

	// [추가] Stage Clear 전에는 진입 불가
	if (IsValid(PortalTrigger))
	{
		PortalTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	OnPortalDeactivated();
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
	// 포탈이 꺼져 있거나 이미 이동을 시작했으면 무시
	if (!bPortalActive || bTravelStarted)
	{
		return;
	}

	// [추가] 플레이어 Pawn만 포탈 사용 가능
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!IsValid(Pawn) || !Pawn->IsPlayerControlled())
	{
		return;
	}

	// 다음 맵이 설정되지 않은 경우 이동하지 않는다.
	if (NextLevelName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGStagePortal] NextLevelName is empty. Portal=%s"),
			*GetName()
		);
		return;
	}

	// [추가] 같은 프레임에 여러 Overlap이 들어와도 OpenLevel은 한 번만 실행
	bTravelStarted = true;
	PortalTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// [추가] 현재 RunState를 Loading으로 전환한 뒤 레벨 이동
	if (ARGGameModeBase* GameMode = Cast<ARGGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->ChangeRunState(ERunState::Loading);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RGStagePortal] Travel: %s -> %s"),
		*UGameplayStatics::GetCurrentLevelName(this, true),
		*NextLevelName.ToString()
	);

	UGameplayStatics::OpenLevel(
		this,
		NextLevelName
	);
}
