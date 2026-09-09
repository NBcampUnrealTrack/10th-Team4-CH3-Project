// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UIManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/HUDWidget.h"
#include "UI/Controller/HUDController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "UI/View/Menu/PauseMenuWidget.h"
#include "Components/OverlaySlot.h"

void AUIManager::BeginPlay()
{
	CreateHUDWidget();
	ApplyGameInputMode();

	Super::BeginPlay();

}

void AUIManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

	HUDWidgetInstance->AddToViewport();
	HUDWidgetInstance->SetLowHealthEffectVisible(bPreviewLowHealthEffect);

	HUDControllerInstance = NewObject<UHUDController>(this);

	if (HUDControllerInstance)
	{
		HUDControllerInstance->Initialize(HUDWidgetInstance);
	}
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




