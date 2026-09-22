#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_BossRader.generated.h"

UCLASS()
class UNREALCH3_API UBTS_BossRader : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTS_BossRader();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

};
