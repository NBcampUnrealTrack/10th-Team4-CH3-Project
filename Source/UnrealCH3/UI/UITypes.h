#pragma once

#include "CoreMinimal.h"
#include "UITypes.generated.h"

UENUM(BlueprintType)
enum class EUILayer : uint8
{
	Game,
	World,
	Notification,
	Selection,
	Menu,
	Transition,
	Result
};