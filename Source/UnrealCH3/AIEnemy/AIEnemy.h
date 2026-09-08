#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIEnemy.generated.h"



UCLASS()
class UNREALCH3_API AAIEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAIEnemy();
	

protected:
	

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
