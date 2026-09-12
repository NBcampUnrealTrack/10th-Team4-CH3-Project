#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RGPlayerAnimInstance.generated.h"


UCLASS()
class UNREALCH3_API URGPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<class ARGCharacter> PlayerCharacter;
	UPROPERTY(BlueprintReadonly, Category = "Character")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Acceleration = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldMove = false;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching = false;
};
