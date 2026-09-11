#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageFeedbackReceiver.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UDamageFeedbackReceiver : public UInterface
{
	GENERATED_BODY()
};

class UNREALCH3_API IDamageFeedbackReceiver
{
	GENERATED_BODY()

public:
	// 대상이 받은 실제 피해량과 처치 여부 수신
	virtual void ReceiveDamageFeedback(
		float AppliedDamage,
		bool bKilled,
		AActor* TargetActor,
		const FVector& WorldLocation
	) {}
};