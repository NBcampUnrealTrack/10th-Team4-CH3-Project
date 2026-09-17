#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EnemyRecoveryProvider.generated.h"

UINTERFACE(BlueprintType)
class UNREALCH3_API UEnemyRecoveryProvider : public UInterface
{
	GENERATED_BODY()
};

class UNREALCH3_API IEnemyRecoveryProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Recovery")
	bool FindRecoveryTransform(AActor* Requester, FTransform& OutRecoveryTransform);
};