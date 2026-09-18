// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGStagePortal.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UDataTable;
class ARGGameModeBase;
class URGProgressionSubsystem;

/**
 * Stage Clear 이후 활성화되는 레벨 이동 포탈.
 *
 * [수정된 흐름]
 *
 * DT_PortalConfig
 *      -> 초기 표시 상태 / 활성 조건 / 전환 지연 / 논리 목적지 로드
 *
 * GameMode OnStageCleared
 *      -> PortalConfig.VisibilityCondition == StageComplete 확인
 *      -> StageConfig.CompletionDestination과 PortalConfig.Destination 비교
 *      -> ActivatePortal()
 *
 * 플레이어 Trigger 진입
 *      -> PortalConfig.TransitionDelaySeconds 대기
 *      -> NextLevelName으로 OpenLevel
 *
 * 중요:
 * DT_PortalConfig의 Destination은 RestHub, NextStageId 같은 "논리 목적지"이고,
 * 실제 Unreal Level Asset 이름은 NextLevelName에 넣는다.
 * Stage 진행 상태를 GameInstance/Subsystem으로 옮기기 전까지는 이 방식이 가장 안전하다.
 */
UCLASS()
class UNREALCH3_API ARGStagePortal : public AActor
{
	GENERATED_BODY()

public:
	ARGStagePortal();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// =========================================================
	// Components
	// =========================================================

	/** 포탈 루트 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Portal|Component"
	)
	TObjectPtr<USceneComponent> PortalRoot;


	/**
	 * 포탈 표시용 Mesh.
	 * BP_RGStagePortal 자식 Blueprint에서 원하는 Mesh/Material을 지정한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Portal|Component"
	)
	TObjectPtr<UStaticMeshComponent> PortalMesh;


	/** 플레이어 진입 감지용 Trigger */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Portal|Component"
	)
	TObjectPtr<UBoxComponent> PortalTrigger;


	// =========================================================
	// [추가] Portal Config DataTable
	// =========================================================

	/**
	 * [추가] 포탈 설정 DataTable.
	 *
	 * Row Struct:
	 * FRGPortalConfigRow
	 *
	 * BP_RGStagePortal Class Defaults에서 DT_PortalConfig를 지정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Config|Portal"
	)
	TObjectPtr<UDataTable> PortalConfigTable;


	/**
	 * [추가] 이 포탈이 사용할 DT_PortalConfig RowName.
	 *
	 * 예:
	 * CombatExit
	 * RestHubExit
	 * BossEntrance
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Config|Portal"
	)
	FName PortalConfigRowName;


	/**
	 * [추가] DataTable에서 읽어온 논리 목적지.
	 *
	 * 예:
	 * RestHub
	 * NextStageId
	 * BossArena
	 *
	 * 실제 Level Asset 이름은 NextLevelName을 사용한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Portal"
	)
	FName ConfiguredDestination;


	/**
	 * [추가] DataTable에서 읽어온 활성/표시 조건.
	 *
	 * 예:
	 * StageComplete
	 * RestHubReady
	 * CoreUpgrade2Applied
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Portal"
	)
	FName VisibilityCondition;


	/**
	 * [추가] DataTable에서 읽어온 전환 지연 시간.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Config|Portal",
		meta = (ClampMin = "0.0", Units = "s")
	)
	float TransitionDelaySeconds;


	/**
	 * [추가] DT_PortalConfig를 읽어서 현재 Portal에 적용.
	 * BeginPlay에서 자동 호출된다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Config|Portal"
	)
	bool ApplyPortalConfig();


	// =========================================================
	// Portal Settings
	// =========================================================

	/**
	 * 실제 Unreal Level Asset 이름.
	 *
	 * 예:
	 * L_Map_TransitHUB
	 * L_Map01_GamePlay
	 * L_Map02_GamePlay02
	 *
	 * DT_PortalConfig의 Destination은 논리 ID이므로,
	 * 현재 단계에서는 실제 OpenLevel 대상만 이 값으로 지정한다.
	 */
	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadWrite,
		Category = "Portal|Travel"
	)
	FName NextLevelName;


	/**
	 * [기존/Fallback]
	 * PortalConfigTable이 지정되지 않았을 때만 사용한다.
	 *
	 * DataTable을 정상 연결했다면 InitialState가 이 값을 대신한다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Portal|State"
	)
	bool bStartActive;


	// =========================================================
	// Portal Control
	// =========================================================

	/** 포탈 활성화 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Portal"
	)
	void ActivatePortal();


	/** 포탈 완전 비활성화 + 숨김 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Portal"
	)
	void DeactivatePortal();


	/**
	 * [추가]
	 * 보이지만 아직 사용할 수 없는 Locked 상태.
	 *
	 * DT_PortalConfig.InitialState == VisibleLocked에서 사용.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Portal"
	)
	void SetPortalVisibleLocked();


	/** 현재 포탈 활성화 여부 */
	UFUNCTION(
		BlueprintPure,
		Category = "Portal"
	)
	bool IsPortalActive() const
	{
		return bPortalActive;
	}


	/**
	 * Blueprint에서 활성화 연출을 추가하고 싶을 때 사용.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Portal|Visual"
	)
	void OnPortalActivated();


	/** Blueprint 비활성화 연출용 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Portal|Visual"
	)
	void OnPortalDeactivated();


	/**
	 * [추가] VisibleLocked 연출용.
	 * 구현하지 않아도 기능에는 영향 없음.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Portal|Visual"
	)
	void OnPortalLockedVisible();


private:
	// =========================================================
	// GameMode Event
	// =========================================================

	/** GameMode의 OnStageCleared를 받는 함수 */
	UFUNCTION()
	void HandleStageCleared();


	/** 플레이어가 PortalTrigger에 들어왔을 때 호출 */
	UFUNCTION()
	void HandlePortalOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);


	/**
	 * [추가] TransitionDelaySeconds가 끝난 후 실제 OpenLevel 수행.
	 */
	UFUNCTION()
	void PerformTravel();


	/**
	 * [추가] 핵심 강화 적용 완료 시 PortalConfig 조건을 다시 평가.
	 */
	UFUNCTION()
	void HandleCoreUpgradeApplied(
		FName UpgradeId,
		int32 ActiveCoreUpgradeCount
	);


	/**
	 * [추가] StageComplete 외의 PortalConfig 조건 평가.
	 *
	 * RestHubReady       -> RestHub 출구 즉시 사용 가능
	 * CoreUpgrade2Applied -> 핵심 강화 2개 보유 시 Boss 입구 활성
	 */
	void EvaluateNonStagePortalCondition();


	/**
	 * [추가]
	 * StageComplete 조건의 Portal이 현재 Stage 목적지와 일치하는지 확인.
	 *
	 * StageConfig.CompletionDestination
	 *      ↕
	 * PortalConfig.Destination
	 */
	bool DoesStageDestinationMatch() const;


private:
	/** 현재 포탈 활성화 상태 */
	bool bPortalActive;


	/** OpenLevel 중복 호출 방지 */
	bool bTravelStarted;


	/** [추가] PortalConfig가 정상 적용되었는지 */
	bool bPortalConfigApplied;


	/** [추가] DataTable에서 읽은 InitialState 값 저장 */
	uint8 CachedInitialState;


	/** [추가] DataTable에서 읽은 InteractionType 값 저장 */
	uint8 CachedInteractionType;


	/** [추가] 레벨 이동 지연용 타이머 */
	FTimerHandle TransitionTimerHandle;


	/** 바인딩 해제를 위한 GameMode 참조 */
	TWeakObjectPtr<ARGGameModeBase> BoundGameMode;

	/** [추가] CoreUpgrade 조건 감시용 Progression Subsystem */
	TWeakObjectPtr<URGProgressionSubsystem> BoundProgression;
};
