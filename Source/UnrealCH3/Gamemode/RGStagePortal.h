// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGStagePortal.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class ARGGameModeBase;

/**
 * Stage Clear 이후 활성화되는 레벨 이동 포탈.
 *
 * 흐름:
 * GameMode OnStageCleared
 *      -> ActivatePortal()
 *      -> 플레이어가 Trigger 진입
 *      -> NextLevelName으로 OpenLevel
 *
 * GameMode를 직접 수정하지 않고 기존 OnStageCleared Delegate를 구독한다.
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Component")
	TObjectPtr<USceneComponent> PortalRoot;

	/**
	 * 포탈 표시용 Mesh.
	 * BP_RGStagePortal 자식 Blueprint에서 원하는 Mesh/Material을 지정하면 된다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Component")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	/** 플레이어 진입 감지용 Trigger */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Component")
	TObjectPtr<UBoxComponent> PortalTrigger;


	// =========================================================
	// Portal Settings
	// =========================================================

	/**
	 * [설정] 이 포탈을 타면 이동할 다음 레벨 이름.
	 *
	 * 예:
	 * L_Map01_GamePlay
	 * L_Map02_GamePlay02
	 *
	 * 레벨에 배치한 Portal 인스턴스의 Details에서 설정한다.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Portal|Travel")
	FName NextLevelName;

	/**
	 * [설정] Stage Clear 전부터 포탈을 켜 둘지 여부.
	 * 전투 맵에서는 false 유지.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|State")
	bool bStartActive;


	// =========================================================
	// Portal Control
	// =========================================================

	/** 포탈 활성화 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void ActivatePortal();

	/** 포탈 비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void DeactivatePortal();

	/** 현재 포탈 활성화 여부 */
	UFUNCTION(BlueprintPure, Category = "Portal")
	bool IsPortalActive() const
	{
		return bPortalActive;
	}

	/**
	 * Blueprint에서 활성화 연출을 추가하고 싶을 때 사용.
	 * 예: Niagara, 사운드, Material 변경 등.
	 * 구현하지 않아도 포탈 기능 자체는 정상 동작한다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|Visual")
	void OnPortalActivated();

	/** Blueprint 비활성화 연출용 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|Visual")
	void OnPortalDeactivated();


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


private:
	/** 현재 포탈 활성화 상태 */
	bool bPortalActive;

	/** OpenLevel 중복 호출 방지 */
	bool bTravelStarted;

	/** 바인딩 해제를 위한 GameMode 참조 */
	TWeakObjectPtr<ARGGameModeBase> BoundGameMode;
};
