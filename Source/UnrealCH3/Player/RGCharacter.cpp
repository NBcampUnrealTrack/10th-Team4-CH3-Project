#include "Player/RGCharacter.h"
#include "RGPlayerController.h"
#include "EnhancedInputComponent.h"
#include "GameFrameWork/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ARGCharacter::ARGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 0.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 0.0f);
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	DashTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Timeline"));
	JumpMaxCount = 2;
	GetCharacterMovement()->MaxWalkSpeed = 800.0f;
	GetCharacterMovement()->JumpZVelocity = 800.0f;
}

void ARGCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (DashTimeline == nullptr || DashCurve == nullptr) return;

	FOnTimelineFloat TimelineFloat;
	TimelineFloat.BindDynamic(this, &ARGCharacter::OnDashUpdate);
	DashTimeline->AddInterpFloat(DashCurve, TimelineFloat);
	DashTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineEvent TimelineFinished;
	TimelineFinished.BindDynamic(this, &ARGCharacter::OnDashFinished);
	DashTimeline->SetTimelineFinishedFunc(TimelineFinished);
	SetCheckPoint();
}

void ARGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateMovementState();

}

void ARGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		ARGPlayerController* PlayerController = Cast<ARGPlayerController>(GetController());
		
		if (!PlayerController) return;
		
		if (PlayerController->MoveAction)
		{
			EnhancedInput->BindAction(PlayerController->MoveAction, ETriggerEvent::Triggered, this, &ARGCharacter::Move);
		}
		if (PlayerController->LookAction)
		{
			EnhancedInput->BindAction(PlayerController->LookAction, ETriggerEvent::Triggered, this, &ARGCharacter::Look);
		}
		if (PlayerController->JumpAction)
		{
			EnhancedInput->BindAction(PlayerController->JumpAction, ETriggerEvent::Started, this, &ARGCharacter::StartJump);
			EnhancedInput->BindAction(PlayerController->JumpAction, ETriggerEvent::Completed, this, &ARGCharacter::StopJump);
		}
		if (PlayerController->DashAction)
		{
			EnhancedInput->BindAction(PlayerController->DashAction, ETriggerEvent::Triggered, this, &ARGCharacter::Dash);
		}
		if (PlayerController->ShootAction)
		{
			EnhancedInput->BindAction(PlayerController->ShootAction, ETriggerEvent::Triggered, this, &ARGCharacter::Shoot);
		}
		if (PlayerController->AimAction)
		{
			EnhancedInput->BindAction(PlayerController->AimAction, ETriggerEvent::Triggered, this, &ARGCharacter::ToggleAim);
		}
	}

}

float ARGCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DmageEvent, AController* EventIntigator, AActor* DamageCauser)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
	}

	return DamageAmount;
}

void ARGCharacter::AddHealth(float Amount)
{
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
}

void ARGCharacter::SetCheckPoint()
{
	if (GetActorLocation().Z > 1.0f || CurrentMovementState == EMovementState::Falling || CurrentMovementState == EMovementState::WallRunning)
	{
		return;
	}

	CheckPoint = GetActorLocation();
}

void ARGCharacter::UpdateMovementState()
{
	if (DashTimeline && DashTimeline->IsPlaying())
	{
		CurrentMovementState = EMovementState::Dashing;
		return;
	}

	if (bIsWallRunning)
	{
		CurrentMovementState = EMovementState::WallRunning;
		return;
	}

	if (GetCharacterMovement()->IsFalling())
	{
		CurrentMovementState = EMovementState::Falling;
	}

	if (!FMath::IsNearlyZero(GetVelocity().SizeSquared2D()))
	{
		CurrentMovementState = EMovementState::Waking;
	}
	else
	{
		CurrentMovementState = EMovementState::Idle;
	}
}

void ARGCharacter::ComebackCheckPoint()
{
	// 초기화 및 체크포인트로 이동

}

void ARGCharacter::Move(const FInputActionValue& value)
{
	MoveInput = value.Get<FVector2D>();

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(GetActorForwardVector(), MoveInput.Y);
	}
	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(GetActorRightVector(), MoveInput.X);
	}
}

void ARGCharacter::Look(const FInputActionValue& value)
{
	FVector2D LookInput = value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void ARGCharacter::StartJump(const FInputActionValue& value)
{
	if (DashTimeline && DashTimeline->IsPlaying())
	{
		DashTimeline->Stop();

		FVector JumpDirection = FMath::IsNearlyZero(MoveInput.X) ? GetActorForwardVector() * MoveInput.Y : GetActorRightVector() * MoveInput.X;

		GetCharacterMovement()->Velocity += JumpDirection * DashDistance;
	}
	Jump();
}

void ARGCharacter::StopJump(const FInputActionValue& value)
{
	StopJumping();
	
}

void ARGCharacter::Dash(const FInputActionValue& value)
{
	if (DashCount <= 0 || CurrentMovementState != EMovementState::Waking || CurrentMovementState == EMovementState::Dashing)
	{
		return;
	}

	FVector DashDirection = FMath::IsNearlyZero(MoveInput.X) ? GetActorForwardVector() * MoveInput.Y : GetActorRightVector() * MoveInput.X;

	DashStartLocation = GetActorLocation();
	DashEndLocation = DashStartLocation + (DashDirection * DashDistance);

	DashTimeline->PlayFromStart();
	DashCount--;
	GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &ARGCharacter::ResetDashCount, DashCooldown, false);
}

void ARGCharacter::Shoot(const FInputActionValue& value)
{
	// 총발사
}

void ARGCharacter::ToggleAim(const FInputActionValue& value)
{
	if (bIsAiming)
	{
		Camera->FieldOfView = 90.0f;
		bIsAiming = false;
	}
	else
	{
		Camera->FieldOfView = 60.0f;
		bIsAiming = true;
	}
}

void ARGCharacter::Reload(const FInputActionValue& value)
{
	// 재장전
}

void ARGCharacter::OnDashUpdate(float Alpha)
{
	FVector CurrentLocation = FMath::Lerp(DashStartLocation, DashEndLocation, Alpha);
	SetActorLocation(CurrentLocation, true, &DashHitResult);
}

void ARGCharacter::OnDashFinished()
{
	CurrentMovementState = EMovementState::Idle;
}

