#include "Player/RGCharacter.h"
#include "RGPlayerController.h"
#include "EnhancedInputComponent.h"
#include "GameFrameWork/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RGBaseWeapon.h"
#include "Gamemode/RGGameModeBase.h"

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
	GetCharacterMovement()->MaxWalkSpeed = DefaultSpeed;
	GetCharacterMovement()->MaxAcceleration = DefaultAccelration;
	GetCharacterMovement()->JumpZVelocity = DefaultJumpZVelocity;
	GetCharacterMovement()->AirControl = DefaultAirControl;
	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBreakingDecelerationWalking = GetCharacterMovement()->BrakingDecelerationWalking;
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

	LevelStartLocation = GetActorLocation();
	GetWorldTimerManager().SetTimer(CheckPointTimerHandle, this, &ARGCharacter::SetCheckPoint, CheckPointInterval, true);
}

void ARGCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTimerHandle();

	Super::EndPlay(EndPlayReason);
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
			EnhancedInput->BindAction(PlayerController->DashAction, ETriggerEvent::Started, this, &ARGCharacter::Dash);
		}
		if (PlayerController->SprintAction)
		{
			EnhancedInput->BindAction(PlayerController->SprintAction, ETriggerEvent::Started, this, &ARGCharacter::ToggleSprint);
		}
		if (PlayerController->CrouchAction)
		{
			EnhancedInput->BindAction(PlayerController->CrouchAction, ETriggerEvent::Started, this, &ARGCharacter::StartCrouch);
			EnhancedInput->BindAction(PlayerController->CrouchAction, ETriggerEvent::Completed, this, &ARGCharacter::StopCrouch);
		}
		if (PlayerController->ShootAction)
		{
			EnhancedInput->BindAction(PlayerController->ShootAction, ETriggerEvent::Triggered, this, &ARGCharacter::Shoot);
		}
		if (PlayerController->AimAction)
		{
			EnhancedInput->BindAction(PlayerController->AimAction, ETriggerEvent::Started, this, &ARGCharacter::ToggleAim);
		}
	}

}

float ARGCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DmageEvent, AController* EventIntigator, AActor* DamageCauser)
{
	if (bIsGodMode || bIsDead)
	{
		return 0.0f;
	}
	// 1초 재생타이머 정리
	GetWorldTimerManager().ClearTimer(TickRegenerationTimerHandle);
	// 5초 타이머 시작
	GetWorldTimerManager().SetTimer(StartRegenerationTimerHandle, this, &ARGCharacter::StartRegenerateHealth, StartRegenerationDelay, false);
	
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		Dead();
	}

	return DamageAmount;
}

int32 ARGCharacter::GetSlopeType()
{
	FFindFloorResult Floor = GetCharacterMovement()->CurrentFloor;

	if (Floor.IsWalkableFloor())
	{
		FVector FloorNormal = Floor.HitResult.ImpactNormal;

		if (FMath::IsNearlyEqual(FloorNormal.Z, 1.0f))
		{
			return 0;
		}

		FVector Velocity2D = GetVelocity().GetSafeNormal2D();

		float DotProduct = FVector::DotProduct(Velocity2D, FloorNormal);

		if (DotProduct > 0.1f)
		{
			return 1;
		}
		else if (DotProduct < -0.1f)
		{
			return 2;
		}
	}

	return 0;
}

void ARGCharacter::AddHealth(float Amount)
{
	if (bIsDead)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
}

void ARGCharacter::SetCheckPoint()
{
	if (GetActorLocation().Z > 1.0f || CurrentMovementState == EMovementState::Falling || CurrentMovementState == EMovementState::WallRunning)
	{
		return;
	}

	CheckPointLocation = GetActorLocation();
}

void ARGCharacter::SetAimState(bool bCanAim)
{
	bIsAiming = bCanAim;
	Camera->FieldOfView = bIsAiming ? 60.0f : 90.0f;
}

void ARGCharacter::SetSprintState(bool bCanSprint)
{
	bIsSprinting = bCanSprint;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : DefaultSpeed;
}

void ARGCharacter::StartRegenerateHealth()
{
	GetWorldTimerManager().SetTimer(TickRegenerationTimerHandle, this, &ARGCharacter::TickRegenerateHealth, TickRegenerationInterval, true);
}

void ARGCharacter::TickRegenerateHealth()
{
	float HealthAmount = MaxHealth * RegenerationMultipiler;
	AddHealth(HealthAmount);

	if (CurrentHealth >= MaxHealth)
	{
		GetWorldTimerManager().ClearTimer(TickRegenerationTimerHandle);
	}
}

void ARGCharacter::ClearTimerHandle()
{
	GetWorldTimerManager().ClearTimer(DashCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(CheckPointTimerHandle);
	GetWorldTimerManager().ClearTimer(GodModeTimerHandle);
	GetWorldTimerManager().ClearTimer(StartRegenerationTimerHandle);
	GetWorldTimerManager().ClearTimer(TickRegenerationTimerHandle);
}

void ARGCharacter::ResetAllState()
{
	UnCrouch();
	SetAimState(false);
	SetSprintState(false);
	GetCharacterMovement()->StopMovementImmediately();
	// TODO 장전등 애니메이션 초기화
}

void ARGCharacter::UpdateMovementState()
{
	if (CurrentMovementState == EMovementState::Sliding)
	{
		float CurrentSpeed = GetVelocity().Size2D();
		int32 Slope = GetSlopeType();
		float ExitSpeed = Slope == 2 ? 550.0f : 450.0f;

		if (CurrentSpeed <= ExitSpeed || GetCharacterMovement()->IsFalling())
		{
			StopSliding();
		}

		return;
	}

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

	if (bIsSprinting)
	{
		CurrentMovementState = EMovementState::Sprinting;
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

void ARGCharacter::ReturnCheckPoint()
{
	// 낙사 데미지
	float FallDamage = MaxHealth * 0.2;
	UGameplayStatics::ApplyDamage(this, FallDamage, GetController(), this, UDamageType::StaticClass());

	if (CurrentHealth == 0.0f)
	{
		Dead();
		return;
	}

	// 체크포인트 이동
	FVector ReturnLocation = CheckPointLocation == FVector::ZeroVector ? LevelStartLocation : CheckPointLocation;
	SetActorLocation(ReturnLocation);

	// 상태 초기화
	ResetAllState();

	// 무적
	bIsGodMode = true;
	TWeakObjectPtr<ARGCharacter> WeakPtr = this;
	GetWorldTimerManager().SetTimer(GodModeTimerHandle, [WeakPtr]() { if (WeakPtr.IsValid()) { WeakPtr.Get()->bIsGodMode = false; } }, GodModeDuration, false);
}

void ARGCharacter::Dead()
{
	bIsDead = true;
	// TODO 죽었을때 로직
	ResetAllState();
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

		GetCharacterMovement()->Velocity = DashVelocity;
	}
	Jump();
	SetSprintState(false);
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

void ARGCharacter::ToggleSprint(const FInputActionValue& value)
{
	if (bIsAiming)
	{
		return;
	}
	SetSprintState(!bIsSprinting);
}

void ARGCharacter::StartCrouch(const FInputActionValue& value)
{
	Crouch();
	if (GetVelocity().Size2D() > 700.0f && CurrentMovementState != EMovementState::Sliding)
	{
		StartSliding();
	}
}

void ARGCharacter::StopCrouch(const FInputActionValue& value)
{
	UnCrouch();
}

void ARGCharacter::Shoot(const FInputActionValue& value)
{
	// TODO 총의 발사 함수 불러오기

}

void ARGCharacter::ToggleAim(const FInputActionValue& value)
{
	// 에임시작시 질주 풀기
	SetSprintState(false);

	SetAimState(!bIsAiming);
}

void ARGCharacter::Reload(const FInputActionValue& value)
{
	// TODO 총의 재장전 함수 불러오기

}

void ARGCharacter::StartSliding()
{
	CurrentMovementState = EMovementState::Sliding;
	int32 Slope = GetSlopeType();

	float SpeedMultipiler = 1.05f;
	float SlideFriction = 0.25f;
	if (Slope == 1)
	{
		SpeedMultipiler = 1.12f;
		SlideFriction = 0.15f;
	}
	else if (Slope == 2)
	{
		SpeedMultipiler = 1.0f;
		SlideFriction = 0.55;
	}

	GetCharacterMovement()->GroundFriction = SlideFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = 100.0f;

	FVector CurrentVelocity = GetVelocity();
	GetCharacterMovement()->Velocity = CurrentVelocity * SpeedMultipiler;
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("Sliding Apply")));
}

void ARGCharacter::StopSliding()
{
	CurrentMovementState = EMovementState::Idle;
	GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = DefaultBreakingDecelerationWalking;
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Sliding Unapply")));
}

void ARGCharacter::StartWallRun()
{

}

void ARGCharacter::StopWallRun()
{

}

void ARGCharacter::OnDashUpdate(float Alpha)
{
	FVector PreviousLocation = GetActorLocation();
	FVector CurrentLocation = FMath::Lerp(DashStartLocation, DashEndLocation, Alpha);
	// 속력 구하기 (이동한거리 / 시간)
	DashVelocity = (CurrentLocation - PreviousLocation) / GetWorld()->DeltaTimeSeconds;

	SetActorLocation(CurrentLocation, true, &DashHitResult);
}

void ARGCharacter::OnDashFinished()
{
	CurrentMovementState = EMovementState::Idle;
}

bool ARGCharacter::InitializeDefaultWeapon()
{
	if (EquippedWeapon)
	{
		return true;
	}

	if (!DefaultWeaponClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[RGCharacter] DefaultWeaponClass is not assigned.")
		);

		return false;
	}

	FActorSpawnParameters SpawnParams;

	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedWeapon =
		GetWorld()->SpawnActor<ARGBaseWeapon>(
			DefaultWeaponClass,
			GetActorTransform(),
			SpawnParams
		);

	if (!EquippedWeapon)
	{
		return false;
	}

	EquippedWeapon->SetOwningCharacter(this);

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		EquippedWeapon->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("WeaponSocket")
		);
	}

	EquippedWeapon->SetExternalActionsAllowed(
		bPlayerActionsAllowed
	);

	return true;
}


void ARGCharacter::SetPlayerActionsAllowed(
	bool bAllowed
)
{
	bPlayerActionsAllowed = bAllowed;

	if (!bAllowed)
	{
		SetSprintState(false);
		SetAimState(false);

		if (DashTimeline)
		{
			DashTimeline->Stop();
		}

		GetCharacterMovement()->StopMovementImmediately();
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->SetExternalActionsAllowed(
			bAllowed
		);
	}
}