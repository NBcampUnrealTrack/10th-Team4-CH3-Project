#include "Component/RGWallRunMovement.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

URGWallRunMovement::URGWallRunMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}

void URGWallRunMovement::BeginPlay()
{
	Super::BeginPlay();
	CharacterOwner = Cast<ACharacter>(GetOwner());
	if (CharacterOwner)
	{
		CharacterMovementComponent = CharacterOwner->GetCharacterMovement();
		CharacterCameraComponent = Cast<UCameraComponent>(CharacterOwner->GetComponentByClass(UCameraComponent::StaticClass()));
		CharacterOwner->LandedDelegate.AddDynamic(this, &URGWallRunMovement::OnCharacterLanded);
	}

	if (CharacterMovementComponent)
	{
		DefaultGravity = CharacterMovementComponent->GravityScale;
		CanWallRunDistance = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + 10.0f;
	}
}

void URGWallRunMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CharacterMovementComponent->IsFalling())
	{
		if (CharacterMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.0f)
		{
			if (bIsWallRunning)
			{
				UpdateWallRun();
			}
			else
			{
				CheckWallRun();
			}
		}
		else
		{
			if (bIsWallRunning)
			{
				StopWallRun();
			}
		}
	}

	UpdateCameraTilt(DeltaTime);
}

void URGWallRunMovement::OnCharacterLanded(const FHitResult& Hit)
{
	LastWallNormal = FVector::ZeroVector;
	LastWallRunLocation = FVector::ZeroVector;
}

void URGWallRunMovement::CheckWallRun()
{
	if (GetWorld()->GetTimeSeconds() < NextCanWallRunTime)
	{
		return;
	}

	FVector Start = CharacterOwner->GetActorLocation();
	FVector ForwardVector = CharacterOwner->GetActorForwardVector();
	FVector RightVector = CharacterOwner->GetActorRightVector();

	FVector LeftEnd = Start + (-RightVector * 70.0f) + (ForwardVector * -30.0f);
	FVector RightEnd = Start + (RightVector * 70.0f) + (ForwardVector * -30.0f);

	FHitResult LeftHit;
	FHitResult RightHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CharacterOwner);

	bool bLeftHit = GetWorld()->LineTraceSingleByChannel(LeftHit, Start, LeftEnd, ECC_Visibility, Params);
	bool bRightHit = GetWorld()->LineTraceSingleByChannel(RightHit, Start, RightEnd, ECC_Visibility, Params);

	if (bLeftHit && LeftHit.Distance <= CanWallRunDistance)
	{
		if (FVector::DotProduct(ForwardVector, LeftHit.ImpactNormal) < -0.95f)
		{
			if (bIsWallRunning)
			{
				StopWallRun();
				return;
			}
		}
		if (FVector::DotProduct(LeftHit.ImpactNormal, LastWallNormal) >= 0.9f)
		{
			if (FVector::DistSquared2D(Start, LastWallRunLocation) < FMath::Square(1300.0f))
			{
				return;
			}
		}

		StartWallRun(LeftHit, -1);
	}
	else if (bRightHit && RightHit.Distance <= CanWallRunDistance)
	{
		if (FVector::DotProduct(ForwardVector, RightHit.ImpactNormal) < -0.95f)
		{
			if (bIsWallRunning)
			{
				StopWallRun();
				return;
			}
		}
		if (FVector::DotProduct(RightHit.ImpactNormal, LastWallNormal) >= 0.9f)
		{
			if (FVector::DistSquared2D(Start, LastWallRunLocation) < FMath::Square(1300.0f))
			{
				return;
			}
		}

		StartWallRun(RightHit, 1);
	}
}

void URGWallRunMovement::StartWallRun(const FHitResult& Hit, int32 Wall)
{
	bIsWallRunning = true;
	CharacterMovementComponent->GravityScale = 0.0f;
	CharacterMovementComponent->Velocity.Z = 0.0f;
	CharacterOwner->JumpCurrentCount = 0;
	WallNormal = Hit.ImpactNormal;
	StartWallRunDirection = FVector::VectorPlaneProject(CharacterOwner->GetActorForwardVector(), WallNormal).GetSafeNormal();
	TargetRollRotation = (Wall == 1) ? -20.0f : 20.0f;
	
	OnWallRunStarted.Broadcast();

	if (CharacterMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(WallRunTimerHandle, this, &URGWallRunMovement::StopWallRun, WallRunTime, false);
	}
	else
	{
		StopWallRun();
	}
}

void URGWallRunMovement::StopWallRun()
{
	bIsWallRunning = false;
	GetWorld()->GetTimerManager().ClearTimer(WallRunTimerHandle);
	CharacterMovementComponent->GravityScale = DefaultGravity;
	LastWallNormal = WallNormal;
	LastWallRunLocation = CharacterOwner->GetActorLocation();
	TargetRollRotation = 0.0f;
	OnWallRunStopped.Broadcast();
}

void URGWallRunMovement::WallJump()
{
	if (!bIsWallRunning)
	{
		return;
	}

	StopWallRun();
	NextCanWallRunTime = GetWorld()->GetTimeSeconds() + 0.3f;
	FVector JumpVelocity = (WallNormal * 600.0f) + (FVector::UpVector * 400.0f) + (CharacterOwner->GetActorForwardVector() * 200.0f);
	CharacterOwner->LaunchCharacter(JumpVelocity, true, true);
}

void URGWallRunMovement::UpdateWallRun()
{
	FVector Start = CharacterOwner->GetActorLocation();
	WallRunSpeed = CharacterMovementComponent->MaxWalkSpeed;
	if (bIsWallRunning)
	{
		FVector End = Start + (-WallNormal * 100.0f);
		FHitResult MaintainHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(CharacterOwner);

		bool bHit = GetWorld()->LineTraceSingleByChannel(MaintainHit, Start, End, ECC_Visibility, Params);
		if (!bHit || MaintainHit.Distance > CanWallRunDistance)
		{
			StopWallRun();
			return;
		}
		FVector ForwardDir = FVector::VectorPlaneProject(CharacterOwner->GetActorForwardVector(), WallNormal).GetSafeNormal();
		if (FVector::DotProduct(StartWallRunDirection, ForwardDir) <= 0.0f)
		{
			StopWallRun();
			return;
		}

		FVector PushDir = -WallNormal;
		FVector FinalVelocity = (ForwardDir * WallRunSpeed) + (PushDir * 250.0f);
		CharacterOwner->LaunchCharacter(FinalVelocity, true, false);
	}
}

void URGWallRunMovement::UpdateCameraTilt(float DeltaTime)
{
	if (CharacterCameraComponent)
	{
		FRotator CurrentRotation = CharacterCameraComponent->GetRelativeRotation();
		float NewRollRotation = FMath::FInterpTo(CurrentRotation.Roll, TargetRollRotation, DeltaTime, 10.0f);
		CurrentRotation.Roll = NewRollRotation;
		CharacterCameraComponent->SetRelativeRotation(CurrentRotation);
	}
}