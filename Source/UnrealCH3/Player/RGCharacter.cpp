#include "Player/RGCharacter.h"

#include "RGPlayerController.h"
#include "EnhancedInputComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/GameplayStatics.h"

#include "RGBaseWeapon.h"
#include "RGAssaultRifle.h"


ARGCharacter::ARGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm =
		CreateDefaultSubobject<USpringArmComponent>(
			TEXT("SpringArm")
		);

	SpringArm->SetupAttachment(RootComponent);

	SpringArm->TargetArmLength = 0.0f;

	SpringArm->SetRelativeLocation(
		FVector(
			0.0f,
			0.0f,
			70.0f
		)
	);

	SpringArm->SocketOffset =
		FVector::ZeroVector;

	SpringArm->bUsePawnControlRotation = true;


	Camera =
		CreateDefaultSubobject<UCameraComponent>(
			TEXT("Camera")
		);

	Camera->SetupAttachment(SpringArm);

	Camera->bUsePawnControlRotation = false;


	DashTimeline =
		CreateDefaultSubobject<UTimelineComponent>(
			TEXT("dAshTimeline")
		);

	AimTimeline =
		CreateDefaultSubobject<UTimelineComponent>(
			TEXT("AimTimeline")
		);

	CrouchTimeline =
		CreateDefaultSubobject<UTimelineComponent>(
			TEXT("CrouchTimeline")
		);


	JumpMaxCount = 2;

	GetCharacterMovement()
		->GetNavAgentPropertiesRef()
		.bCanCrouch = true;

	GetCharacterMovement()
		->MaxWalkSpeed = DefaultSpeed;

	GetCharacterMovement()
		->MaxAcceleration = DefaultAccelration;

	GetCharacterMovement()
		->JumpZVelocity = DefaultJumpZVelocity;

	GetCharacterMovement()
		->AirControl = DefaultAirControl;

	DefaultGroundFriction =
		GetCharacterMovement()
		->GroundFriction;

	DefaultBreakingDecelerationWalking =
		GetCharacterMovement()
		->BrakingDecelerationWalking;
}


void ARGCharacter::BeginPlay()
{
	Super::BeginPlay();


	// -----------------------------------------------------
	// Dash Timeline
	// -----------------------------------------------------

	if (DashTimeline && DashCurve)
	{
		FOnTimelineFloat DashTimelineFloat;

		DashTimelineFloat.BindDynamic(
			this,
			&ARGCharacter::OnDashUpdate
		);

		DashTimeline->AddInterpFloat(
			DashCurve,
			DashTimelineFloat
		);

		DashTimeline->SetTimelineLengthMode(
			ETimelineLengthMode::TL_LastKeyFrame
		);


		FOnTimelineEvent DashTimelineFinished;

		DashTimelineFinished.BindDynamic(
			this,
			&ARGCharacter::OnDashFinished
		);

		DashTimeline->SetTimelineFinishedFunc(
			DashTimelineFinished
		);
	}


	// -----------------------------------------------------
	// Aim Timeline
	// -----------------------------------------------------

	if (AimTimeline && AimCurve)
	{
		FOnTimelineFloat AimTimelineFloat;

		AimTimelineFloat.BindDynamic(
			this,
			&ARGCharacter::OnAimUpdate
		);

		AimTimeline->AddInterpFloat(
			AimCurve,
			AimTimelineFloat
		);

		AimTimeline->SetTimelineLengthMode(
			ETimelineLengthMode::TL_LastKeyFrame
		);
	}


	// -----------------------------------------------------
	// Crouch Timeline
	// -----------------------------------------------------

	if (CrouchTimeline && CrouchCurve)
	{
		FOnTimelineFloat CrouchTimelineFloat;

		CrouchTimelineFloat.BindDynamic(
			this,
			&ARGCharacter::OnCrouchCameraUpdate
		);

		CrouchTimeline->AddInterpFloat(
			CrouchCurve,
			CrouchTimelineFloat
		);

		CrouchTimeline->SetTimelineLengthMode(
			ETimelineLengthMode::TL_LastKeyFrame
		);
	}


	// -----------------------------------------------------
	// 체크포인트
	// -----------------------------------------------------

	LevelStartLocation =
		GetActorLocation();

	GetWorldTimerManager().SetTimer(
		CheckPointTimerHandle,
		this,
		&ARGCharacter::SetCheckPoint,
		CheckPointInterval,
		true
	);


	// -----------------------------------------------------
	// 시작 무기
	// -----------------------------------------------------

	if (WeaponClass)
	{
		EquipWeapon(
			WeaponClass
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGCharacter] WeaponClass is None. "
				"Using ARGAssaultRifle fallback."
			)
		);

		EquipWeapon(
			ARGAssaultRifle::StaticClass()
		);
	}
}


void ARGCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	ClearTimerHandle();

	Super::EndPlay(
		EndPlayReason
	);
}


void ARGCharacter::Tick(
	float DeltaTime
)
{
	Super::Tick(
		DeltaTime
	);
}


void ARGCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent
)
{
	Super::SetupPlayerInputComponent(
		PlayerInputComponent
	);


	UEnhancedInputComponent* EnhancedInput =
		Cast<UEnhancedInputComponent>(
			PlayerInputComponent
		);

	if (!EnhancedInput)
	{
		return;
	}


	ARGPlayerController* PlayerController =
		Cast<ARGPlayerController>(
			GetController()
		);

	if (!PlayerController)
	{
		return;
	}


	if (PlayerController->MoveAction)
	{
		EnhancedInput->BindAction(
			PlayerController->MoveAction,
			ETriggerEvent::Triggered,
			this,
			&ARGCharacter::Move
		);

		EnhancedInput->BindAction(
			PlayerController->MoveAction,
			ETriggerEvent::Completed,
			this,
			&ARGCharacter::StopMove
		);
	}


	if (PlayerController->LookAction)
	{
		EnhancedInput->BindAction(
			PlayerController->LookAction,
			ETriggerEvent::Triggered,
			this,
			&ARGCharacter::Look
		);
	}


	if (PlayerController->JumpAction)
	{
		EnhancedInput->BindAction(
			PlayerController->JumpAction,
			ETriggerEvent::Started,
			this,
			&ARGCharacter::StartJump
		);

		EnhancedInput->BindAction(
			PlayerController->JumpAction,
			ETriggerEvent::Completed,
			this,
			&ARGCharacter::StopJump
		);
	}


	if (PlayerController->DashAction)
	{
		EnhancedInput->BindAction(
			PlayerController->DashAction,
			ETriggerEvent::Started,
			this,
			&ARGCharacter::Dash
		);
	}


	if (PlayerController->SprintAction)
	{
		EnhancedInput->BindAction(
			PlayerController->SprintAction,
			ETriggerEvent::Started,
			this,
			&ARGCharacter::ToggleSprint
		);
	}


	if (PlayerController->CrouchAction)
	{
		EnhancedInput->BindAction(
			PlayerController->CrouchAction,
			ETriggerEvent::Started,
			this,
			&ARGCharacter::ToggleCrouch
		);
	}


	if (PlayerController->FireAction)
	{
		EnhancedInput->BindAction(
			PlayerController->FireAction,
			ETriggerEvent::Started,
			this,
			&ARGCharacter::StartFire
		);

		EnhancedInput->BindAction(
			PlayerController->FireAction,
			ETriggerEvent::Completed,
			this,
			&ARGCharacter::StopFire
		);
	}


	if (PlayerController->AimAction)
	{
		EnhancedInput->BindAction(
			PlayerController->AimAction,
			ETriggerEvent::Started,
			this,
			&ARGCharacter::StartAim
		);

		EnhancedInput->BindAction(
			PlayerController->AimAction,
			ETriggerEvent::Completed,
			this,
			&ARGCharacter::StopAim
		);
	}
}


float ARGCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DmageEvent,
	AController* EventIntigator,
	AActor* DamageCauser
)
{
	if (
		bIsGodMode ||
		bIsDead
		)
	{
		return 0.0f;
	}


	GetWorldTimerManager().ClearTimer(
		TickRegenerationTimerHandle
	);


	GetWorldTimerManager().SetTimer(
		StartRegenerationTimerHandle,
		this,
		&ARGCharacter::StartRegenerateHealth,
		StartRegenerationDelay,
		false
	);


	CurrentHealth =
		FMath::Clamp(
			CurrentHealth - DamageAmount,
			0.0f,
			MaxHealth
		);


	if (CurrentHealth <= 0.0f)
	{
		Dead();
	}


	return DamageAmount;
}


void ARGCharacter::Falling()
{
	Super::Falling();


	if (
		GetCharacterMovement()->MovementMode ==
		MOVE_Falling
		)
	{
		if (
			CurrentMovementState ==
			EMovementState::Sliding
			)
		{
			StopSliding();
		}


		CurrentMovementState =
			EMovementState::Falling;


		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"CurrentState : %d"
			),
			static_cast<int32>(
				CurrentMovementState
				)
		);
	}
}


void ARGCharacter::Landed(
	const FHitResult& Hit
)
{
	Super::Landed(
		Hit
	);


	if (
		CurrentMovementState ==
		EMovementState::Dashing ||
		CurrentMovementState ==
		EMovementState::Sliding
		)
	{
		return;
	}


	CurrentMovementState =
		EMovementState::Idle;


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"CurrentState : %d"
		),
		static_cast<int32>(
			CurrentMovementState
			)
	);
}


bool ARGCharacter::CanJumpInternal_Implementation() const
{
	bool bCanJump =
		Super::CanJumpInternal_Implementation();


	if (
		bIsCrouched ||
		CurrentMovementState ==
		EMovementState::Sliding
		)
	{
		bCanJump = true;
	}


	return bCanJump;
}


int32 ARGCharacter::GetSlopeType()
{
	FFindFloorResult Floor =
		GetCharacterMovement()
		->CurrentFloor;


	if (Floor.IsWalkableFloor())
	{
		const FVector FloorNormal =
			Floor.HitResult.ImpactNormal;


		if (
			FMath::IsNearlyEqual(
				FloorNormal.Z,
				1.0f
			)
			)
		{
			return 0;
		}


		const FVector Velocity2D =
			GetVelocity()
			.GetSafeNormal2D();


		const float DotProduct =
			FVector::DotProduct(
				Velocity2D,
				FloorNormal
			);


		if (DotProduct > 0.1f)
		{
			return 1;
		}

		if (DotProduct < -0.1f)
		{
			return 2;
		}
	}


	return 0;
}


void ARGCharacter::AddHealth(
	float Amount
)
{
	if (bIsDead)
	{
		return;
	}


	CurrentHealth =
		FMath::Clamp(
			CurrentHealth + Amount,
			0.0f,
			MaxHealth
		);
}


void ARGCharacter::SetCheckPoint()
{
	if (
		GetActorLocation().Z > 1.0f ||
		CurrentMovementState ==
		EMovementState::Falling ||
		CurrentMovementState ==
		EMovementState::WallRunning
		)
	{
		return;
	}


	CheckPointLocation =
		GetActorLocation();
}


void ARGCharacter::SetAimState(
	bool bCanAim
)
{
	if (
		bIsAiming ==
		bCanAim
		)
	{
		return;
	}


	bIsAiming =
		bCanAim;


	if (bIsAiming)
	{
		SetSprintState(
			false
		);


		if (CurrentWeapon)
		{
			CurrentWeapon->StartAiming();
		}


		if (AimTimeline)
		{
			AimTimeline->Play();
		}
	}
	else
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StopAiming();
		}


		if (AimTimeline)
		{
			AimTimeline->Reverse();
		}
	}
}


void ARGCharacter::SetSprintState(
	bool bCanSprint
)
{
	bIsSprinting =
		bCanSprint;


	GetCharacterMovement()
		->MaxWalkSpeed =
		bIsSprinting
		? SprintSpeed
		: DefaultSpeed;
}


void ARGCharacter::StartRegenerateHealth()
{
	GetWorldTimerManager().SetTimer(
		TickRegenerationTimerHandle,
		this,
		&ARGCharacter::TickRegenerateHealth,
		TickRegenerationInterval,
		true
	);
}


void ARGCharacter::TickRegenerateHealth()
{
	const float HealthAmount =
		MaxHealth *
		RegenerationMultipiler;


	AddHealth(
		HealthAmount
	);


	if (
		CurrentHealth >=
		MaxHealth
		)
	{
		GetWorldTimerManager().ClearTimer(
			TickRegenerationTimerHandle
		);
	}
}


void ARGCharacter::ClearTimerHandle()
{
	GetWorldTimerManager().ClearTimer(
		DashCooldownTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		CheckPointTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		GodModeTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		StartRegenerationTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		TickRegenerationTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		SlideTimerHandle
	);
}


void ARGCharacter::ResetAllState()
{
	UnCrouch();

	SetAimState(
		false
	);

	SetSprintState(
		false
	);

	GetCharacterMovement()
		->StopMovementImmediately();
}


void ARGCharacter::ReturnCheckPoint()
{
	const float FallDamage =
		MaxHealth * 0.2f;


	UGameplayStatics::ApplyDamage(
		this,
		FallDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);


	if (CurrentHealth <= 0.0f)
	{
		Dead();

		return;
	}


	const FVector ReturnLocation =
		CheckPointLocation ==
		FVector::ZeroVector
		? LevelStartLocation
		: CheckPointLocation;


	SetActorLocation(
		ReturnLocation
	);


	ResetAllState();


	bIsGodMode = true;


	TWeakObjectPtr<ARGCharacter> WeakPtr =
		this;


	GetWorldTimerManager().SetTimer(
		GodModeTimerHandle,
		[WeakPtr]()
		{
			if (WeakPtr.IsValid())
			{
				WeakPtr.Get()
					->bIsGodMode = false;
			}
		},
		GodModeDuration,
		false
	);
}


void ARGCharacter::CheckSlideSpeed()
{
	if (
		GetVelocity().Size2D() <
		StopSlideSpeed
		)
	{
		StopSliding();
	}
}


void ARGCharacter::Dead()
{
	if (bIsDead)
	{
		return;
	}


	bIsDead = true;


	ResetAllState();
}


void ARGCharacter::EquipWeapon(
	TSubclassOf<ARGBaseWeapon> SpawnWeaponClass
)
{
	if (!SpawnWeaponClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGCharacter] EquipWeapon failed: "
				"SpawnWeaponClass is null."
			)
		);

		return;
	}


	UWorld* World =
		GetWorld();


	if (!World)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGCharacter] EquipWeapon failed: "
				"World is null."
			)
		);

		return;
	}


	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();

		CurrentWeapon = nullptr;
	}


	WeaponClass =
		SpawnWeaponClass;


	FActorSpawnParameters SpawnParams;

	SpawnParams.Owner =
		this;

	SpawnParams.Instigator =
		this;

	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


	CurrentWeapon =
		World->SpawnActor<ARGBaseWeapon>(
			WeaponClass,
			FTransform::Identity,
			SpawnParams
		);


	if (!CurrentWeapon)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[RGCharacter] Weapon spawn failed."
			)
		);

		return;
	}


	CurrentWeapon->SetOwningCharacter(
		this
	);


	const FName WeaponSocketName =
		TEXT("WeaponSocket");


	if (
		GetMesh() &&
		GetMesh()->DoesSocketExist(
			WeaponSocketName
		)
		)
	{
		CurrentWeapon->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::
			SnapToTargetNotIncludingScale,
			WeaponSocketName
		);


		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[RGCharacter] Weapon attached to WeaponSocket. "
				"Weapon=%s"
			),
			*GetNameSafe(
				CurrentWeapon
			)
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[RGCharacter] WeaponSocket not found on character mesh."
			)
		);
	}
}


void ARGCharacter::Move(
	const FInputActionValue& Value
)
{
	MoveInput =
		Value.Get<FVector2D>();


	if (
		CurrentMovementState ==
		EMovementState::Idle ||
		CurrentMovementState ==
		EMovementState::Walking ||
		CurrentMovementState ==
		EMovementState::Sprinting
		)
	{
		const EMovementState NextMovementState =
			bIsSprinting
			? EMovementState::Sprinting
			: EMovementState::Walking;


		if (
			CurrentMovementState !=
			NextMovementState
			)
		{
			CurrentMovementState =
				NextMovementState;
		}
	}


	if (
		!FMath::IsNearlyZero(
			MoveInput.Y
		)
		)
	{
		AddMovementInput(
			GetActorForwardVector(),
			MoveInput.Y
		);
	}


	if (
		!FMath::IsNearlyZero(
			MoveInput.X
		)
		)
	{
		AddMovementInput(
			GetActorRightVector(),
			MoveInput.X
		);
	}
}


void ARGCharacter::StopMove(
	const FInputActionValue& Value
)
{
	SetSprintState(
		false
	);


	if (
		CurrentMovementState ==
		EMovementState::Walking ||
		CurrentMovementState ==
		EMovementState::Sprinting
		)
	{
		CurrentMovementState =
			EMovementState::Idle;
	}
}


void ARGCharacter::Look(
	const FInputActionValue& Value
)
{
	const FVector2D LookInput =
		Value.Get<FVector2D>();


	AddControllerYawInput(
		LookInput.X
	);

	AddControllerPitchInput(
		LookInput.Y
	);
}


void ARGCharacter::StartJump(
	const FInputActionValue& Value
)
{
	if (
		DashTimeline &&
		DashTimeline->IsPlaying()
		)
	{
		DashTimeline->Stop();

		GetCharacterMovement()
			->Velocity =
			DashVelocity;
	}


	if (
		CurrentMovementState ==
		EMovementState::Sliding
		)
	{
		UnCrouch();

		StopSliding();
	}


	Jump();
}


void ARGCharacter::StopJump(
	const FInputActionValue& Value
)
{
	StopJumping();
}


void ARGCharacter::Dash(
	const FInputActionValue& Value
)
{
	if (
		DashCount <= 0 ||
		GetVelocity().IsNearlyZero() ||
		CurrentMovementState ==
		EMovementState::Dashing ||
		!DashTimeline
		)
	{
		return;
	}


	if (
		CurrentMovementState ==
		EMovementState::Sliding
		)
	{
		UnCrouch();

		StopSliding();
	}


	CurrentMovementState =
		EMovementState::Dashing;


	FVector DashDirection =
		FMath::IsNearlyZero(
			MoveInput.X
		)
		? GetActorForwardVector() *
		MoveInput.Y
		: GetActorRightVector() *
		MoveInput.X;


	DashStartLocation =
		GetActorLocation();


	DashEndLocation =
		DashStartLocation +
		(
			DashDirection *
			DashDistance
			);


	DashTimeline->PlayFromStart();


	DashCount--;


	GetWorldTimerManager().SetTimer(
		DashCooldownTimerHandle,
		this,
		&ARGCharacter::ResetDashCount,
		DashCooldown,
		false
	);
}


void ARGCharacter::ToggleSprint(
	const FInputActionValue& Value
)
{
	if (bIsAiming)
	{
		return;
	}


	SetSprintState(
		!bIsSprinting
	);
}


void ARGCharacter::ToggleCrouch(
	const FInputActionValue& Value
)
{
	if (
		CurrentMovementState ==
		EMovementState::Falling
		)
	{
		return;
	}


	if (bIsCrouched)
	{
		UnCrouch();


		if (CrouchTimeline)
		{
			CrouchTimeline->Reverse();
		}


		if (
			CurrentMovementState ==
			EMovementState::Sliding
			)
		{
			StopSliding();
		}
	}
	else
	{
		Crouch();


		if (CrouchTimeline)
		{
			CrouchTimeline->Play();
		}


		if (
			GetVelocity().Size2D() >
			700.0f &&
			CurrentMovementState !=
			EMovementState::Sliding
			)
		{
			StartSliding();
		}
	}
}


void ARGCharacter::StartFire(
	const FInputActionValue& Value
)
{
	if (
		bIsDead ||
		!CurrentWeapon
		)
	{
		return;
	}


	CurrentWeapon->StartFire();
}


void ARGCharacter::StopFire(
	const FInputActionValue& Value
)
{
	if (
		bIsDead ||
		!CurrentWeapon
		)
	{
		return;
	}


	CurrentWeapon->StopFire();
}


void ARGCharacter::StartAim(
	const FInputActionValue& Value
)
{
	if (
		bIsDead ||
		!CurrentWeapon
		)
	{
		return;
	}


	SetAimState(
		true
	);
}


void ARGCharacter::StopAim(
	const FInputActionValue& Value
)
{
	if (
		bIsDead ||
		!CurrentWeapon
		)
	{
		return;
	}


	SetAimState(
		false
	);
}


void ARGCharacter::Reload(
	const FInputActionValue& Value
)
{
	if (
		bIsDead ||
		!CurrentWeapon
		)
	{
		return;
	}


	CurrentWeapon->StartReloaded();
}


void ARGCharacter::StartSliding()
{
	CurrentMovementState =
		EMovementState::Sliding;


	const int32 Slope =
		GetSlopeType();


	float SpeedMultipiler =
		1.05f;

	float SlideFriction =
		0.25f;


	if (Slope == 1)
	{
		SpeedMultipiler =
			1.12f;

		SlideFriction =
			0.15f;
	}
	else if (Slope == 2)
	{
		SpeedMultipiler =
			1.0f;

		SlideFriction =
			0.55f;
	}


	GetCharacterMovement()
		->MaxWalkSpeedCrouched =
		GetCharacterMovement()
		->MaxWalkSpeed;


	GetCharacterMovement()
		->MaxAcceleration =
		0.0f;


	GetCharacterMovement()
		->GroundFriction =
		SlideFriction;


	GetCharacterMovement()
		->BrakingDecelerationWalking =
		0.0f;


	GetCharacterMovement()
		->Velocity *=
		SpeedMultipiler;


	GetWorldTimerManager().SetTimer(
		SlideTimerHandle,
		this,
		&ARGCharacter::CheckSlideSpeed,
		0.1f,
		true
	);
}


void ARGCharacter::StopSliding()
{
	GetCharacterMovement()
		->MaxWalkSpeedCrouched =
		300.0f;


	GetCharacterMovement()
		->MaxAcceleration =
		DefaultAccelration;


	GetCharacterMovement()
		->GroundFriction =
		DefaultGroundFriction;


	GetCharacterMovement()
		->BrakingDecelerationWalking =
		DefaultBreakingDecelerationWalking;


	GetWorldTimerManager().ClearTimer(
		SlideTimerHandle
	);


	CurrentMovementState =
		EMovementState::Idle;
}


void ARGCharacter::StartWallRun()
{
}


void ARGCharacter::StopWallRun()
{
}


void ARGCharacter::OnDashUpdate(
	float Alpha
)
{
	const FVector PreviousLocation =
		GetActorLocation();


	const FVector CurrentLocation =
		FMath::Lerp(
			DashStartLocation,
			DashEndLocation,
			Alpha
		);


	if (
		GetWorld() &&
		!FMath::IsNearlyZero(
			GetWorld()->DeltaTimeSeconds
		)
		)
	{
		DashVelocity =
			(
				CurrentLocation -
				PreviousLocation
				) /
			GetWorld()->DeltaTimeSeconds;
	}


	SetActorLocation(
		CurrentLocation,
		true,
		&DashHitResult
	);
}


void ARGCharacter::OnDashFinished()
{
	CurrentMovementState =
		EMovementState::Idle;
}


void ARGCharacter::OnAimUpdate(
	float Alpha
)
{
	if (!CurrentWeapon)
	{
		return;
	}


	const float TargetFOV =
		CurrentWeapon->GetADSFOV();


	const float CurrentFOV =
		FMath::Lerp(
			TargetFOV,
			DefaultFOV,
			Alpha
		);


	Camera->SetFieldOfView(
		CurrentFOV
	);
}


void ARGCharacter::OnCrouchCameraUpdate(
	float Alpha
)
{
	const float TargetOffsetZ =
		FMath::Lerp(
			0.0f,
			CrouchCapsuleValue,
			Alpha
		);


	FVector CurrentOffset =
		SpringArm->SocketOffset;


	CurrentOffset.Z =
		TargetOffsetZ;


	SpringArm->SocketOffset =
		CurrentOffset;
}