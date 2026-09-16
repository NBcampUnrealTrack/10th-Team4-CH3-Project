#include "Player/RGPlayerController.h"
#include "Gamemode/RGProgressionSubsystem.h"
#include "EnhancedInputSubsystems.h"

ARGPlayerController::ARGPlayerController()
	: DefaultIMC(nullptr)
	, MoveAction(nullptr)
	, LookAction(nullptr)
	, JumpAction(nullptr)
	, DashAction(nullptr)
	, SprintAction(nullptr)
	, CrouchAction(nullptr)
	, GrappleAction(nullptr)
	, FireAction(nullptr)
	, AimAction(nullptr)
	, ReloadAction(nullptr)
{
}

void ARGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultIMC)
			{
				Subsystem->AddMappingContext(DefaultIMC, 0);
			}
		}
	}
}

void ARGPlayerController::DebugForceLevelUp()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>())
		{
			Progression->DebugForceLevelUp();
		}
	}
}