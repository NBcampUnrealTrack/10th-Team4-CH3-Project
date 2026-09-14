#include "Player/Animation/RGPlayerAnimInstance.h"
#include "RGPlayerAnimInstance.h"
#include "Player/RGCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void URGPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (PlayerCharacter = Cast<ARGCharacter>(TryGetPawnOwner()))
	{
		CharacterMovementComponent = PlayerCharacter->GetCharacterMovement();
	}
}

void URGPlayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!PlayerCharacter || !CharacterMovementComponent)
	{
		return;
	}

	Velocity = PlayerCharacter->GetVelocity();
	Acceleration = CharacterMovementComponent->GetCurrentAcceleration().Size2D();
	GroundSpeed = Velocity.Size2D();
	bShouldMove = Acceleration > 0.0f;
	bIsFalling = CharacterMovementComponent->IsFalling();
	bIsSprinting = PlayerCharacter->GetCurrentMovementState() == EMovementState::Sprinting;
	bIsCrouching = CharacterMovementComponent->IsCrouching();
}
