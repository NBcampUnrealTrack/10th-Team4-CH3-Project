// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/UITypes.h"
// [추가] RunFlow 정책 Delegate 타입
#include "Gamemode/DataTableStruct/RGRunConfigRows.h"
#include "UIManager.generated.h"

class UHUDWidget;
class UHUDController;
class UUserWidget;
class UWeaponInfoWidget;
class ARGBaseWeapon;
class UCrosshairWidget;
class ARGCharacter;
class ARGGameModeBase;
class URailgunChargeWidget;
class URGCraftingStationWidget;
class URGQuickSlotWidget;

/**
 *
 */
UCLASS()
class UNREALCH3_API AUIManager : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	//실제 화면
	UPROPERTY(Transient)
	TObjectPtr<UHUDWidget> HUDWidgetInstance;

	//화면에 값 전달
	UPROPERTY(Transient)
	TObjectPtr<UHUDController> HUDControllerInstance;

	//메뉴 입력 모드
	UPROPERTY(Transient)
	bool bIsMenuInputModeActive = false;

	//테스트용 변수 모음
	UPROPERTY(EditDefaultsOnly, Category = "UI|Debug")
	bool bPreviewLowHealthEffect = false;

	UPROPERTY(EditDefaultsOnly, Category = "UI|View")
	TSubclassOf<UUserWidget> PauseMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Debug")
	TSubclassOf<UUserWidget> TestSelectionClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|View")
	TSubclassOf<UUserWidget> CraftingStationClass;

	UFUNCTION()
	void HandleResumeRequested();

	//View 저장 Map 생성
	UPROPERTY(Transient)
	TMap<TSubclassOf<UUserWidget>, TObjectPtr<UUserWidget>> ActiveViews;

	//View Layer 저장 Map 생성
	UPROPERTY(Transient)
	TMap<TSubclassOf<UUserWidget>, EUILayer> ActiveViewLayers;

	//해당 Layer 게임 입력 Blocking 판단
	bool IsInputBlockingLayer(EUILayer Layer) const;

	//입력을 막는 Layer 존재 여부 판단
	bool HasInputBlockingView() const;

	//Layer 에 활성화 된 View 존재 여부 판단
	bool HasActiveViewInLayer(EUILayer Layer) const;

	// Controll 에서 확정된 피해 정보를 BP로 전달
	void HandleDamageNumberRequested(
		float AppliedDamage,
		AActor* TargetActor,
		FVector WorldLocation
	);

	// 플레이어 캐릭터의 무기 장착 이벤트에 연결
	void TryBindPlayerWeaponSource();

	// EndPlay 시 Character Delegate 연결 정리
	void UnbindPlayerWeaponSource();

	// Character가 새 무기를 장착했을 때 HUDController에 실제 무기 연결
	UFUNCTION()
	void HandlePlayerWeaponEquipped(ARGBaseWeapon* NewWeapon);

	// 현재 구독 중인 플레이어 캐릭터
	TWeakObjectPtr<ARGCharacter> BoundPlayerCharacter;

	// GameMode RunFlow 정책 연결
	void TryBindRunFlowSource();
	void UnbindRunFlowSource();

	UFUNCTION()
	void HandleRunFlowPolicyChanged(
		ERGRunInputPolicy InputPolicy,
		ERGRunTimePolicy TimePolicy,
		ERGRunAIState AIState,
		FName TopUI
	);

	TWeakObjectPtr<ARGGameModeBase> BoundRunFlowGameMode;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Damage")
	void OnDamageNumberDisplayRequested(
		float AppliedDamage,
		AActor* TargetActor,
		FVector WorldLocation
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|RunFlow")
	void OnRunFlowTopUIChanged(FName TopUI);

	//공격 경고 표시
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|CombatFeedback")
	void OnAttackWarningDisplayRequested(
		AActor* Attacker,
		float WarningDuration
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|CombatFeedback")
	void OnAttackWarningHideReqested(AActor* Attacker);

	//피격 방향 표시
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|CombatFeedback")
	void OnDirectionDamageDisplayRequested(
		AActor* Attacker,
		FVector AttackOrigin
	);

public:
	// 테스트용 함수 모음
	UFUNCTION(Exec, BlueprintCallable, Category = "UI|Debug")
	void TestOpenPauseMenu();

	// 저체력 HUD 갱신
	UFUNCTION(Exec, BlueprintCallable, Category = "UI|Health")
	void TestLowHealthEffect(
		float CurrentHealth,
		float MaxHealth
	);

	UFUNCTION(Exec)
	void TestOpenSelection();

	UFUNCTION(Exec)
	void TestCloseSelection();

	UFUNCTION(Exec, BlueprintCallable, Category = "UI|Debug")
	void TestOpenCraftingStation();

	UFUNCTION(Exec, BlueprintCallable, Category = "UI|Debug")
	void TestCloseCraftingStation();

	//HUD 생성
	UFUNCTION(Exec, BlueprintCallable, Category = "UI")
	void CreateHUDWidget();

	//HUD 제거
	UFUNCTION(Exec)
	void RemoveHUDWidget();

	//HUD 표시 (Move, Look 사용)
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void ApplyGameInputMode();

	//일시정지, 강화, 결과 화면 등 표시
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void ApplyMenuInputMode();

	//View 계층 추가
	UFUNCTION(BlueprintCallable, Category = "UI|View")
	UUserWidget* OpenView(
		TSubclassOf<UUserWidget> ViewClass,
		EUILayer Layer
	);

	//View 계층 제거
	UFUNCTION(BlueprintCallable, Category = "UI|View")
	bool CloseView(
		TSubclassOf<UUserWidget> ViewClass
	);

	// 생성된 무기 정보 View를 기존 HUDController에 등록
	UFUNCTION(BlueprintCallable, Category = "UI|Binding")
	void RegisterWeaponInfoView(
		UWeaponInfoWidget* InWeaponInfoView
	);

	// 장착한 무기 정보 UI Controller에 전달
	UFUNCTION(BlueprintCallable, Category = "UI|Binding")
	void SetEquippedWeapon(
		ARGBaseWeapon* InWeapon,
		const FText& InWeaponDisplayName
	);

	UFUNCTION(BlueprintCallable, Category = "UI|Binding")
	void RegisterCrosshairView(
		UCrosshairWidget* InCrosshairView
	);

	//공격 준비 시작
	UFUNCTION(BlueprintCallable, Category = "UI|CombatFeedback")
	void NotifyAttackWarningStarted(
		AActor* Attacker,
		float WarningDuration
	);

	//공격 취소
	UFUNCTION(BlueprintCallable, Category = "UI|CombatFeedback")
	void NotifyAttackWarningCanceled(
		AActor* Attacker
	);

	//피격 방향
	UFUNCTION(BlueprintCallable, Category = "UI|CombatFeedback")
	void NotifyDirectionalDamage(
		AActor* Attacker,
		FVector AttackOrigin
	);

	UFUNCTION(BlueprintCallable, Category = "UI|Binding")
	void RegisterRailgunChargeView(
		URailgunChargeWidget* InRailgunChargeView
	);

	UFUNCTION()
	void RegisterQuickSlotView(
		URGQuickSlotWidget* InHealQuickSlotView,
		URGQuickSlotWidget* InGrenadeQuickSlotView
	);
};