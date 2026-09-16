#pragma once

#include "CoreMinimal.h"
#include "Enemy/BaseEnemy.h"
#include "BossEnemy.generated.h"


UCLASS()
class UNREALCH3_API ABossEnemy : public ABaseEnemy
{
	GENERATED_BODY()
	
public:
	ABossEnemy();

	virtual void Attack() override;
};
