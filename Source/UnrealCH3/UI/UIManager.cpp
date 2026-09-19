// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UIManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/HUDWidget.h"
#include "UI/Controller/HUDController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "UI/View/Menu/PauseMenuWidget.h"
#include "Components/OverlaySlot.h"
#include "UI/View/Combat/WeaponInfoWidget.h"
#include "UI/View/Combat/CrosshairWidget.h"
// [추가] 플레이어의 실제 장착 무기 Delegate에 연결하기 위해 사용
#include "Player/RGCharacter.h"
#include "RGBaseWeapon.h"
// [추가] RunFlow TopUI 정책 수신
#include "Gamemode/RGGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "UI/View/Combat/RailgunChargeWidget.h"

void AUIManager::BeginPlay()
{
	CreateHUDWidget();
	ApplyGameInputMode();

	Super::BeginPlay();

	// [추가] Super::BeginPlay 이후에 연결한다.
	// 이 시점에는 BP_CombatUIManager의 BeginPlay가 끝난 뒤이므로 Crosshair/WeaponInfo View 등록도 완료되어 있다.
	// Character의 BeginPlay가 먼저 끝났다면 CurrentWeapon을 즉시 바인딩하고,
	// 아직 무기 생성 전이라면 OnWeaponEquipped Delegate가 이후 자동으로 처리한다.
	TryBindPlayerWeaponSource();

	// [추가] 현재 RunFlow TopUI 정책도 자동 연결
	TryBindRunFlowSource();
}

void AUIManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// [추가] Character Delegate부터 안전하게 해제한다.
	UnbindPlayerWeaponSource();

	UE_LOG(LogTemp,
		Log,
		TEXT("[UIManager] EndPlay - Remove HUD Widget")
	);

	RemoveHUDWidget();
	Super::EndPlay(EndPlayReason);
}

void AUIManager::HandleResumeRequested()
{
	CloseView(PauseMenuClass);
}

bool AUIManager::IsInputBlockingLayer(EUILayer Layer) const
{
	switch (Layer)
	{
	case EUILayer::Selection:
	case EUILayer::Menu:	
	case EUILayer::Transition:		
	case EUILayer::Result:
		return true;

	default:
		return false;
	}
}

bool AUIManager::HasInputBlockingView() const
{
	//const TPair : Map의 값을 복사하지 않고 읽기 전용으로 확인
	//ViewLayer.Value : 저장된 EUILayer 위의 계층값중 하나라도 발견되면 트루로 반환
	for (const TPair<TSubclassOf<UUserWidget>, EUILayer>& ViewLayer : ActiveViewLayers)
	{
		if (IsInputBlockingLayer(ViewLayer.Value))
		{
			return true;
		}
	}
	return false;
}

bool AUIManager::HasActiveViewInLayer(EUILayer Layer) const
{
	for (const TPair<TSubclassOf<UUserWidget>, EUILayer>& ViewLayer : ActiveViewLayers)
	{
		if (ViewLayer.Value != Layer)
		{
			continue;
		}

		const TObjectPtr<UUserWidget>* FoundView =
			ActiveViews.Find(ViewLayer.Key);

		if (FoundView && IsValid(FoundView->Get()))
		{
			return true;
		}
	}

	return false;
}

void AUIManager::HandleDamageNumberRequested(float AppliedDamage, AActor* TargetActor, FVector WorldLocation)
{
	OnDamageNumberDisplayRequested(AppliedDamage, TargetActor, WorldLocation);
}

// [추가] 플레이어 캐릭터와 무기 장착 이벤트 연결
void AUIManager::TryBindPlayerWeaponSource()
{
	if (!IsValid(PlayerOwner))
	{
		return;
	}

	ARGCharacter* PlayerCharacter = Cast<ARGCharacter>(PlayerOwner->GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	// 이미 같은 캐릭터에 연결되어 있다면 Delegate를 중복 등록하지 않는다.
	if (BoundPlayerCharacter.Get() != PlayerCharacter)
	{
		UnbindPlayerWeaponSource();

		BoundPlayerCharacter = PlayerCharacter;
		PlayerCharacter->OnWeaponEquipped.AddUniqueDynamic(
			this,
			&AUIManager::HandlePlayerWeaponEquipped
		);
	}

	// Character가 UIManager보다 먼저 BeginPlay를 끝낸 경우를 처리한다.
	// 이미 생성된 CurrentWeapon이 있으면 지금 즉시 HUDController에 바인딩한다.
	if (ARGBaseWeapon* CurrentWeapon = PlayerCharacter->GetCurrentWeapon())
	{
		HandlePlayerWeaponEquipped(CurrentWeapon);
	}
}

// [추가] 플레이어 캐릭터 Delegate 연결 해제
void AUIManager::UnbindPlayerWeaponSource()
{
	if (ARGCharacter* PlayerCharacter = BoundPlayerCharacter.Get())
	{
		PlayerCharacter->OnWeaponEquipped.RemoveDynamic(
			this,
			&AUIManager::HandlePlayerWeaponEquipped
		);
	}

	BoundPlayerCharacter.Reset();
}

// [추가] 새 무기가 실제 장착된 순간 HUDController에 연결
void AUIManager::HandlePlayerWeaponEquipped(ARGBaseWeapon* NewWeapon)
{
	if (!IsValid(HUDControllerInstance) || !IsValid(NewWeapon))
	{
		return;
	}

	// 별도의 BP Text 입력을 강제하지 않도록 클래스 이름을 기본 표시명으로 사용한다.
	// 표시명은 UI 용도일 뿐, 히트/킬/데미지 피드백 동작에는 영향을 주지 않는다.
	FString WeaponDisplayName = NewWeapon->GetClass()->GetName();
	WeaponDisplayName.RemoveFromStart(TEXT("BP_"));
	WeaponDisplayName.RemoveFromEnd(TEXT("_C"));

	HUDControllerInstance->BindWeapon(
		NewWeapon,
		FText::FromString(WeaponDisplayName)
	);
}


// [추가] GameMode RunFlow 정책 연결
void AUIManager::TryBindRunFlowSource()
{
	ARGGameModeBase* GameMode =
		Cast<ARGGameModeBase>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!IsValid(GameMode))
	{
		return;
	}

	if (BoundRunFlowGameMode.Get() != GameMode)
	{
		UnbindRunFlowSource();

		BoundRunFlowGameMode = GameMode;

		GameMode->OnRunFlowPolicyChanged.AddUniqueDynamic(
			this,
			&AUIManager::HandleRunFlowPolicyChanged
		);
	}

	// BeginPlay 순서와 관계없이 현재 TopUI를 즉시 동기화
	OnRunFlowTopUIChanged(
		GameMode->GetCurrentTopUI()
	);
}


void AUIManager::UnbindRunFlowSource()
{
	if (
		ARGGameModeBase* GameMode =
			BoundRunFlowGameMode.Get()
		)
	{
		GameMode->OnRunFlowPolicyChanged.RemoveDynamic(
			this,
			&AUIManager::HandleRunFlowPolicyChanged
		);
	}

	BoundRunFlowGameMode.Reset();
}


void AUIManager::HandleRunFlowPolicyChanged(
	ERGRunInputPolicy InputPolicy,
	ERGRunTimePolicy TimePolicy,
	ERGRunAIState AIState,
	FName TopUI
)
{
	// 입력 모드와 시간은 GameMode가 이미 실제 적용한다.
	// UIManager는 자신이 담당하는 TopUI 논리 ID만 BP에 전달한다.
	OnRunFlowTopUIChanged(
		TopUI
	);
}


void AUIManager::TestOpenPauseMenu()
{
	if (!PauseMenuClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("PauseMenuClass is not assigned.")
		);
		return;
	}
	//IsChildOf : 설정한 클래스가 UPauseMenuWidget 을 상속했는지 확인, StaticClass: C++ 클래스의 언리얼 타입 정보를 가져온다.
	if (!PauseMenuClass->IsChildOf(UPauseMenuWidget::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("PauseMenuClass must inherit from PauseMenuWidget.")
		);
		return;
	}

	UUserWidget* View = OpenView(PauseMenuClass, EUILayer::Menu);
	UPauseMenuWidget* PauseMenu = Cast<UPauseMenuWidget>(View);

	if (!IsValid(PauseMenu))
	{
		return;
	}

	PauseMenu->OnResumeRequested.AddUniqueDynamic(this, &AUIManager::HandleResumeRequested);
}

void AUIManager::TestLowHealthEffect(float CurrentHealth, float MaxHealth)
{
	if (!HUDControllerInstance)
	{
		UE_LOG(LogTemp,
			Warning,
			TEXT("HUDController is not ready")
		);

		return;
	}

	HUDControllerInstance->HandleHealthChanged(CurrentHealth, MaxHealth);

}

void AUIManager::TestOpenSelection()
{
	if (!TestSelectionClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("TestSelectionClass is not assigned")
		);
		return;
	}

	OpenView(TestSelectionClass, EUILayer::Selection);
}

void AUIManager::TestCloseSelection()
{
	CloseView(TestSelectionClass);
}

void AUIManager::CreateHUDWidget()
{
	if (!HUDWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("HUDWidgetClass is not assigned")
		);
		return;
	}

	if (IsValid(HUDWidgetInstance))
	{
		return;
	}

	if (!IsValid(PlayerOwner))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UIManager has no PlayerOwner")
		);
		return;
	}

	HUDWidgetInstance =
		CreateWidget<UHUDWidget>(PlayerOwner.Get(), HUDWidgetClass);

	if (!IsValid(HUDWidgetInstance))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Failed to create HUDWidget")
		);
		return;
	}

	HUDWidgetInstance->SetLowHealthEffectVisible(
		bPreviewLowHealthEffect
	);

	HUDControllerInstance =
		NewObject<UHUDController>(this);

	if (!IsValid(HUDControllerInstance))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Failed to create HUDController")
		);

		HUDWidgetInstance = nullptr;
		return;
	}

	HUDControllerInstance->Initialize(
		HUDWidgetInstance
	);

	HUDControllerInstance
		->OnDamageNumberRequested
		.AddUObject(
			this,
			&AUIManager::HandleDamageNumberRequested
		);

	HUDWidgetInstance->AddToViewport();

}

void AUIManager::RemoveHUDWidget()
{
	//추가 된 view 반복 검색 후 제거
	for (TPair<TSubclassOf<UUserWidget>, TObjectPtr<UUserWidget>>& ViewPair : ActiveViews)
	{
		if (IsValid(ViewPair.Value.Get()))
		{
			ViewPair.Value->RemoveFromParent();
		}
	}

	//View,Later 초기화
	ActiveViews.Empty();
	ActiveViewLayers.Empty();

	//RemoveHUDWidget 호출 시 메인메뉴 입력 모드일 시 게임 입력모드로 전환
	if (bIsMenuInputModeActive)
	{
		ApplyGameInputMode();
	}

	if (HUDControllerInstance)
	{
		HUDControllerInstance->OnDamageNumberRequested.RemoveAll(this);

		HUDControllerInstance->Shutdown();
		HUDControllerInstance = nullptr;
	}

	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}
}

void AUIManager::ApplyGameInputMode()
{
	if (!IsValid(PlayerOwner))
	{
		UE_LOG(LogTemp,
			Warning,
			TEXT("UIManager has no PlayerOwner.")
		);
		return;
	}

	FInputModeGameOnly InputMode;
	PlayerOwner->SetInputMode(InputMode);
	PlayerOwner->SetShowMouseCursor(false);

	if (bIsMenuInputModeActive)
	{
		PlayerOwner->SetIgnoreMoveInput(false);
		PlayerOwner->SetIgnoreLookInput(false);
		bIsMenuInputModeActive = false;
	}
}

void AUIManager::ApplyMenuInputMode()
{
	if (!IsValid(PlayerOwner))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UIManager has no PlayerOwner")
		);
		return;
	}

	if (bIsMenuInputModeActive)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	PlayerOwner->SetInputMode(InputMode);
	PlayerOwner->SetShowMouseCursor(true);
	PlayerOwner->SetIgnoreMoveInput(true);
	PlayerOwner->SetIgnoreLookInput(true);

	bIsMenuInputModeActive = true;
}

UUserWidget* AUIManager::OpenView(TSubclassOf<UUserWidget> ViewClass, EUILayer Layer)
{
	if (!ViewClass || !IsValid(HUDWidgetInstance))
	{
		return nullptr;
	}



	if (TObjectPtr<UUserWidget>* ExistingView =
		ActiveViews.Find(ViewClass))
	{
		if (IsValid(ExistingView->Get()))
		{
			return ExistingView->Get();
		}

		ActiveViews.Remove(ViewClass);
	}

	UOverlay* TargetLayer = HUDWidgetInstance->GetLayer(Layer);

	if (!IsValid(TargetLayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Requested UI layer is invalid."));
		return nullptr;
	}

	const bool bSelectionBlockedByMenu =
		Layer == EUILayer::Selection && HasActiveViewInLayer(EUILayer::Menu);

	const bool bMenuBlockedBySelection =
		Layer == EUILayer::Menu && HasActiveViewInLayer(EUILayer::Selection);

	if (bSelectionBlockedByMenu || bMenuBlockedBySelection)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Selection and Menu views cannot be opened together.")
		);

		return nullptr;
	}

	UUserWidget* NewView =
		CreateWidget<UUserWidget>(PlayerOwner.Get(), ViewClass);

	if (!IsValid(NewView))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UI View."));
		return nullptr;
	}

	UOverlaySlot* ViewSlot = TargetLayer->AddChildToOverlay(NewView);

	if (!IsValid(ViewSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to add View to Overlay."));
		return nullptr;
	}

	ViewSlot->SetHorizontalAlignment(HAlign_Fill);
	ViewSlot->SetVerticalAlignment(VAlign_Fill);

	ActiveViews.Add(ViewClass, NewView);
	ActiveViewLayers.Add(ViewClass, Layer);

	if (IsInputBlockingLayer(Layer))
	{
		ApplyMenuInputMode();
	}

	return NewView;
}

bool AUIManager::CloseView(TSubclassOf<UUserWidget> ViewClass)
{
	if (!ViewClass)
	{
		return false;
	}

	TObjectPtr<UUserWidget>* FoundView =
		ActiveViews.Find(ViewClass);

	if (!FoundView)
	{
		return false;
	}

	if (IsValid(FoundView->Get()))
	{
		FoundView->Get()->RemoveFromParent();
	}

	ActiveViews.Remove(ViewClass);
	ActiveViewLayers.Remove(ViewClass);

	if (!HasInputBlockingView())
	{
		ApplyGameInputMode();
	}

	return true;
}

void AUIManager::RegisterWeaponInfoView(UWeaponInfoWidget* InWeaponInfoView)
{
	if (!IsValid(HUDControllerInstance))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UIManager] HUDController is not ready.")
		);
		return;
	}

	if (!IsValid(InWeaponInfoView))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UIManager] WeaponInfo View is invalid.")
		);
		return;
	}

	HUDControllerInstance->SetWeaponInfoView(InWeaponInfoView);
}

void AUIManager::SetEquippedWeapon(ARGBaseWeapon* InWeapon, const FText& InWeaponDisplayName)
{
	if (!IsValid(HUDControllerInstance))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UIManager] Cannot bind weapon: HUDController is not ready.")
		);
		return;
	}

	HUDControllerInstance->BindWeapon(InWeapon, InWeaponDisplayName);
}

void AUIManager::RegisterCrosshairView(UCrosshairWidget* InCrosshairView)
{
	if (!IsValid(HUDControllerInstance) || !IsValid(InCrosshairView))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UIManager] Cannot register crosshair view.")
		);
		return;
	}

	HUDControllerInstance->SetCrosshairView(InCrosshairView);
}

void AUIManager::NotifyAttackWarningStarted(AActor* Attacker, float WarningDuration)
{
	if (!IsValid(HUDWidgetClass))
	{
		return;
	}

	if (!IsValid(Attacker) || !FMath::IsFinite(WarningDuration) || WarningDuration <= 0.0f)
	{
		return;
	}

	OnAttackWarningDisplayRequested(Attacker, WarningDuration);
}

void AUIManager::NotifyAttackWarningCanceled(AActor* Attacker)
{
	if (!IsValid(HUDWidgetInstance))
	{
		return;
	}

	//공격자 사망시 경고 제거 요청
	OnAttackWarningHideReqested(Attacker);
}

void AUIManager::NotifyDirectionalDamage(AActor* Attacker, FVector AttackOrigin)
{
	if (!IsValid(HUDWidgetInstance))
	{
		return;
	}

	if (AttackOrigin.ContainsNaN())
	{
		return;
	}

	//피격 위치만 전달 공격 발생 위치 기준
	AActor* ValidAttacker = IsValid(Attacker) ? Attacker : nullptr;

	OnDirectionDamageDisplayRequested(ValidAttacker, AttackOrigin);
}

void AUIManager::RegisterRailgunChargeView(URailgunChargeWidget* InRailgunChargeView)
{
	if (!IsValid(HUDControllerInstance))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[UIManager] Cannot register railgun charge view: "
				"HUDController is not ready."
			)
		);
		return;
	}

	if (!IsValid(InRailgunChargeView))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[UIManager] Cannot register railgun charge view: "
				"View is invalid."
			)
		);
		return;
	}

	HUDControllerInstance->SetRailgunChargeView(InRailgunChargeView);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RailgunUI] Registered View: %s"),
		*InRailgunChargeView->GetName()
	);
}








