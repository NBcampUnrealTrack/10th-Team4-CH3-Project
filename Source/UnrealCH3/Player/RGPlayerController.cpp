#include "Player/RGPlayerController.h"
#include "EnhancedInputSubsystems.h"

ARGPlayerController::ARGPlayerController()
	: DefaultIMC(nullptr)
	, MoveAction(nullptr)
	, LookAction(nullptr)
	, JumpAction(nullptr)
	, DashAction(nullptr)
	, CrouchAction(nullptr)
	, SprintAction(nullptr)
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
		if (UEnhancedInputLocalPlayerSubsystem* Subsytem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsytem->AddMappingContext(DefaultIMC, 0);
		}
	}
}
