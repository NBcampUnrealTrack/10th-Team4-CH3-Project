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
#include "GameMode/RGGameModeBase.h"
#include "Component/RGWallRunMovement.h"
#include "Component/RGGrappleComponent.h"

ARGCharacter::ARGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 0.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 0.0f);
	SpringArm->bUsePawnControlRotation = true;
	// 카메라와 캡슐사이 장애물이 있을때 줌을 땅겨주는 기능
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	WallRunMovement = CreateDefaultSubobject<URGWallRunMovement>(TEXT("WallRunMovement"));
	WallRunMovement->OnWallRunStarted.AddDynamic(this, &ARGCharacter::OnStartWallRun);
	WallRunMovement->OnWallRunStopped.AddDynamic(this, &ARGCharacter::OnStopWallRun);

	GrappleComponent = CreateDefaultSubobject<URGGrappleComponent>(TEXT("GrappleComponent"));
	GrappleComponent->OnGrappleStarted.AddDynamic(this, &ARGCharacter::OnCanGrapple);
	GrappleComponent->OnGrappleStopped.AddDynamic(this, &ARGCharacter::OnStopGrapple);

	JumpMaxCount = 2;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = DefaultSpeed;
	GetCharacterMovement()->MaxAcceleration = DefaultAccelration;
	GetCharacterMovement()->JumpZVelocity = DefaultJumpZVelocity;
	GetCharacterMovement()->AirControl = DefaultAirControl;
	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBreakingDecelerationWalking = GetCharacterMovement()->BrakingDecelerationWalking;

	DashTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DashTimeline"));
	AimTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("AimTimeline"));
	CrouchTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchTimeline"));

}

void ARGCharacter::BeginPlay()
{
	Super::BeginPlay();
	MeshRelativeLocation = GetMesh()->GetRelativeLocation();

	// 대쉬 타임라인
	if (DashTimeline == nullptr || DashCurve == nullptr)
	{
		return;
	}
	FOnTimelineFloat DashTimelineFloat;
	DashTimelineFloat.BindDynamic(this, &ARGCharacter::OnDashUpdate);
	DashTimeline->AddInterpFloat(DashCurve, DashTimelineFloat);
	DashTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineEvent DashTimelineFinished;
	DashTimelineFinished.BindDynamic(this, &ARGCharacter::OnDashFinished);
	DashTimeline->SetTimelineFinishedFunc(DashTimelineFinished);

	// 에임 타임라인
	if (AimTimeline == nullptr || AimCurve == nullptr)
	{
		return;
	}
	FOnTimelineFloat AimTimelineFloat;
	AimTimelineFloat.BindDynamic(this, &ARGCharacter::OnAimUpdate);
	AimTimeline->AddInterpFloat(AimCurve, AimTimelineFloat);
	AimTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	// Crouch 카메라 타임라인
	if (CrouchTimeline == nullptr || CrouchCurve == nullptr)
	{
		return;
	}
	FOnTimelineFloat CrouchTimelineFloat;
	CrouchTimelineFloat.BindDynamic(this, &ARGCharacter::OnCrouchCameraUpdate);
	CrouchTimeline->AddInterpFloat(CrouchCurve, CrouchTimelineFloat);
	CrouchTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	// 시작위치 셋팅, 체크포인트 타이머 시작
	LevelStartLocation = GetActorLocation();
	GetWorldTimerManager().SetTimer(CheckPointTimerHandle, this, &ARGCharacter::SetCheckPoint, CheckPointInterval, true);

	// 임시로 여기서 불러놨음 UI버튼에서 클릭완료되면 지우기
	EquipWeapon(WeaponClass);
}

void ARGCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTimerHandle();

	Super::EndPlay(EndPlayReason);
}

void ARGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
			EnhancedInput->BindAction(PlayerController->MoveAction, ETriggerEvent::Completed, this, &ARGCharacter::StopMove);
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
			EnhancedInput->BindAction(PlayerController->CrouchAction, ETriggerEvent::Started, this, &ARGCharacter::ToggleCrouch);
		}
		if (PlayerController->GrappleAction)
		{
			EnhancedInput->BindAction(PlayerController->GrappleAction, ETriggerEvent::Started, this, &ARGCharacter::StartGrapple);
		}
		if (PlayerController->FireAction)
		{
			EnhancedInput->BindAction(PlayerController->FireAction, ETriggerEvent::Started, this, &ARGCharacter::StartFire);
			EnhancedInput->BindAction(PlayerController->FireAction, ETriggerEvent::Completed, this, &ARGCharacter::StopFire);
		}
		if (PlayerController->AimAction)
		{
			EnhancedInput->BindAction(PlayerController->AimAction, ETriggerEvent::Started, this, &ARGCharacter::StartAim);
			EnhancedInput->BindAction(PlayerController->AimAction, ETriggerEvent::Completed, this, &ARGCharacter::StopAim);
		}
		if (PlayerController->ReloadAction)
		{
			EnhancedInput->BindAction(PlayerController->ReloadAction, ETriggerEvent::Started, this, &ARGCharacter::Reload);
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
	
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, GetMaxHealthWithUpgrade());

	if (CurrentHealth <= 0.0f)
	{
		Dead();
	}

	return DamageAmount;
}

void ARGCharacter::Falling()
{
	Super::Falling();

	if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		if (CurrentMovementState == EMovementState::Sliding)
		{
			StopSliding();
		}

		SetMovementState(EMovementState::Falling);
	}
}

void ARGCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	SetMovementState(EMovementState::Idle);
}

bool ARGCharacter::CanJumpInternal_Implementation() const
{
	bool bCanJump = Super::CanJumpInternal_Implementation();

	if (bIsCrouched || CurrentMovementState == EMovementState::Sliding)
	{
		bCanJump = true;
	}
	return bCanJump;
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

		if (DotProduct > 0.0f)
		{
			return 1;
		}
		else if (DotProduct < 0.0f)
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

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, GetMaxHealthWithUpgrade());
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
	if (bIsAiming == bCanAim)
	{
		return;
	}

	bIsAiming = bCanAim;

	if (bIsAiming)
	{
		SetSprintState(false);
		CurrentWeapon->StartAiming();
		if (AimTimeline)
		{
			AimTimeline->Play();
		}
	}
	else
	{
		CurrentWeapon->StopAiming();
		if (AimTimeline)
		{
			AimTimeline->Reverse();
		}
	}
	
}

void ARGCharacter::SetSprintState(bool bCanSprint)
{
	bIsSprinting = bCanSprint;

	const float MoveSpeedMultiplier = GetMoveSpeedMultiplier();

	GetCharacterMovement()->MaxWalkSpeed =
		(bIsSprinting ? SprintSpeed : DefaultSpeed)
		* MoveSpeedMultiplier;
}

void ARGCharacter::SetMovementState(EMovementState NewState)
{
	if (CurrentMovementState == NewState)
	{
		return;
	}

	if (GetCharacterMovement()->IsFalling())
	{
		if (NewState == EMovementState::Idle || NewState == EMovementState::Walking || NewState == EMovementState::Sprinting)
		{
			return;
		}
	}

	if (CurrentMovementState == EMovementState::Dashing || CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Grappling)
	{
		if (NewState == EMovementState::Walking || NewState == EMovementState::Sprinting)
		{
			return;
		}
	}

	CurrentMovementState = NewState;
	UE_LOG(LogTemp, Warning, TEXT("Current Movement : %d"), CurrentMovementState);
}

void ARGCharacter::StartRegenerateHealth()
{
	GetWorldTimerManager().SetTimer(TickRegenerationTimerHandle, this, &ARGCharacter::TickRegenerateHealth, TickRegenerationInterval, true);
}

void ARGCharacter::TickRegenerateHealth()
{
	const float HealthAmount = GetRegenerationPerSecond() * TickRegenerationInterval;

	AddHealth(HealthAmount);

	if (CurrentHealth >= GetMaxHealthWithUpgrade())
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
	GetWorldTimerManager().ClearTimer(SlideTimerHandle);
}

void ARGCharacter::ResetAllState()
{
	UnCrouch();
	SetAimState(false);
	SetSprintState(false);
	GetCharacterMovement()->StopMovementImmediately();
	// TODO 장전등 애니메이션 초기화
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

void ARGCharacter::CheckSlideSpeed()
{
	if (GetVelocity().Size2D() < StopSlideSpeed)
	{
		StopSliding();
	}
}

void ARGCharacter::Dead()
{
	bIsDead = true;
	// TODO 죽었을때 로직
	ResetAllState();
	if (ARGGameModeBase* GameMode = Cast<ARGGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->CheckEndCondition(true, false);
	}
}

void ARGCharacter::EquipWeapon(TSubclassOf<ARGBaseWeapon> SpawnWeaponClass)
{
	if (!SpawnWeaponClass)
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	WeaponClass = SpawnWeaponClass;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	CurrentWeapon = GetWorld()->SpawnActor<ARGBaseWeapon>(WeaponClass, FTransform::Identity, SpawnParams);
	if (CurrentWeapon)
	{
		CurrentWeapon->SetOwningCharacter(this);
		CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponSocket"));

		// 애니메이션 바인딩
		CurrentWeapon->OnShotFired.AddDynamic(this, &ARGCharacter::PlayFireAnimation);
		CurrentWeapon->OnShotFiredStop.AddDynamic(this, &ARGCharacter::StopFireAnimation);
		CurrentWeapon->OnReloadStarted.AddDynamic(this, &ARGCharacter::PlayReloadAnimation);
		CurrentWeapon->OnReloadCanceled.AddDynamic(this, &ARGCharacter::StopReloadAnimation);
	}
}

void ARGCharacter::Move(const FInputActionValue& value)
{
	MoveInput = value.Get<FVector2D>();

	EMovementState NextMovementState = bIsSprinting ? EMovementState::Sprinting : EMovementState::Walking;
	SetMovementState(NextMovementState);

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(GetActorForwardVector(), MoveInput.Y);
	}
	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(GetActorRightVector(), MoveInput.X);
	}
}

void ARGCharacter::StopMove(const FInputActionValue& value)
{
	SetSprintState(false);
	SetMovementState(EMovementState::Idle);
}

void ARGCharacter::Look(const FInputActionValue& value)
{
	FVector2D LookInput = value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void ARGCharacter::StartJump(const FInputActionValue& value)
{
	if (CurrentMovementState == EMovementState::WallRunning)
	{
		WallRunMovement->WallJump();
		return;
	}

	if (DashTimeline && DashTimeline->IsPlaying())
	{
		DashTimeline->Stop();

		GetCharacterMovement()->Velocity = DashVelocity;
	}

	StopCrouch();
	Jump();
}

void ARGCharacter::StopJump(const FInputActionValue& value)
{
	StopJumping();
}

void ARGCharacter::Dash(const FInputActionValue& value)
{
	if (DashCount <= 0 || GetVelocity().IsNearlyZero() || CurrentMovementState == EMovementState::Dashing || CurrentMovementState == EMovementState::WallRunning)
	{
		return;
	}

	StopCrouch();
	SetMovementState(EMovementState::Dashing);

	FVector DashDirection = FMath::IsNearlyZero(MoveInput.X) ? GetActorForwardVector() * MoveInput.Y : GetActorRightVector() * MoveInput.X;
	DashStartLocation = GetActorLocation();
	DashEndLocation = DashStartLocation + (DashDirection * DashDistance);

	DashTimeline->PlayFromStart();
	DashCount--;
	GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &ARGCharacter::ResetDashCount, DashCooldown, false);
}

void ARGCharacter::ToggleSprint(const FInputActionValue& value)
{
	if (bIsDead || bIsAiming)
	{
		return;
	}
	SetSprintState(!bIsSprinting);
}

void ARGCharacter::ToggleCrouch(const FInputActionValue& value)
{
	if (CurrentMovementState == EMovementState::Falling || CurrentMovementState == EMovementState::WallRunning)
	{
		return;
	}

	if (bIsCrouched)
	{
		StopCrouch();
	}
	else
	{
		StartCrouch();
	}
}

void ARGCharacter::StartGrapple(const FInputActionValue& value)
{
	if (GrappleComponent)
	{
		GrappleComponent->StartGrapple();
	}
}

void ARGCharacter::OnCanGrapple()
{
	if (CurrentMovementState == EMovementState::WallRunning)
	{
		WallRunMovement->StopWallRun();
	}
	else if (CurrentMovementState == EMovementState::Dashing)
	{
		if (DashTimeline && DashTimeline->IsPlaying())
		{
			DashTimeline->Stop();
		}
	}
	else if (CurrentMovementState == EMovementState::Sliding)
	{
		StopSliding();
	}

	SetMovementState(EMovementState::Grappling);
}

void ARGCharacter::OnStopGrapple()
{
	if (GetCharacterMovement()->IsFalling())
	{
		SetMovementState(EMovementState::Falling);
	}
	else
	{
		SetMovementState(EMovementState::Idle);
	}
}

void ARGCharacter::StartFire(const FInputActionValue& value)
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->StartFire();
}

void ARGCharacter::StopFire(const FInputActionValue& value)
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->StopFire();
}

void ARGCharacter::StartAim(const FInputActionValue& value)
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}
	
	SetAimState(true);
}

void ARGCharacter::StopAim(const FInputActionValue& value)
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}

	SetAimState(false);
}

void ARGCharacter::Reload(const FInputActionValue& value)
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->StartReloaded();
}

void ARGCharacter::StartCrouch()
{
	Crouch();
	if (CrouchTimeline)
	{
		CrouchTimeline->Play();
	}

	if (GetVelocity().Size2D() > 700.0f && CurrentMovementState != EMovementState::Sliding)
	{
		StartSliding();
	}
}

void ARGCharacter::StopCrouch()
{
	UnCrouch();
	if (CrouchTimeline)
	{
		CrouchTimeline->Reverse();
	}

	if (CurrentMovementState == EMovementState::Sliding)
	{
		StopSliding();
	}
}

void ARGCharacter::StartSliding()
{
	SetMovementState(EMovementState::Sliding);

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

	GetCharacterMovement()->MaxWalkSpeedCrouched = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxAcceleration = 0.0f;
	GetCharacterMovement()->GroundFriction = SlideFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
	GetCharacterMovement()->Velocity *= SpeedMultipiler;
	GetWorldTimerManager().SetTimer(SlideTimerHandle, this, &ARGCharacter::CheckSlideSpeed, 0.1f, true);
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("Sliding Apply")));
}

void ARGCharacter::StopSliding()
{
	SetMovementState(EMovementState::Idle);
	GetCharacterMovement()->MaxWalkSpeedCrouched = 300.0f;
	GetCharacterMovement()->MaxAcceleration = DefaultAccelration;
	GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = DefaultBreakingDecelerationWalking;
	GetWorldTimerManager().ClearTimer(SlideTimerHandle);

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Sliding Unapply")));
}

void ARGCharacter::OnStartWallRun()
{
	SetMovementState(EMovementState::WallRunning);
}

void ARGCharacter::OnStopWallRun()
{
	if (GetCharacterMovement()->IsFalling())
	{
		SetMovementState(EMovementState::Falling);
	}
	else
	{
		SetMovementState(EMovementState::Idle);
	}
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
	SetMovementState(EMovementState::Idle);
}

void ARGCharacter::OnAimUpdate(float Alpha)
{
	if (!CurrentWeapon)
	{
		return;
	}

	float TargetFOV = CurrentWeapon->GetADSFOV();
	float CurrentFOV = FMath::Lerp(DefaultFOV, TargetFOV, Alpha);

	Camera->SetFieldOfView(CurrentFOV);
}

void ARGCharacter::OnCrouchCameraUpdate(float Alpha)
{
	float SpringArmTargetZ = FMath::Lerp(0.0f, CrouchCapsuleHeight, Alpha);
	FVector SpringArmCurrentOffset = SpringArm->TargetOffset;
	SpringArmCurrentOffset.Z = SpringArmTargetZ;
	SpringArm->TargetOffset = SpringArmCurrentOffset;

	float MeshLocationZ = MeshRelativeLocation.Z;
	float MeshTargetZ = FMath::Lerp(MeshLocationZ, MeshLocationZ + CrouchCapsuleHeight, Alpha);
	FVector MeshCurrentRelativeLocation = GetMesh()->GetRelativeLocation();
	MeshCurrentRelativeLocation.Z = MeshTargetZ;
	GetMesh()->SetRelativeLocation(MeshCurrentRelativeLocation);
}

void ARGCharacter::PlayFireAnimation()
{
	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
	}
}

void ARGCharacter::StopFireAnimation()
{
	if (FireMontage)
	{
		StopAnimMontage(FireMontage);
	}
}

void ARGCharacter::PlayReloadAnimation()
{
	if (!ReloadMontage || !CurrentWeapon)
	{
		return;
	}

	const float MontageLength = ReloadMontage->GetPlayLength();
	const float ActualReloadTime = CurrentWeapon->GetReloadTime();

	// 방어 코드: 몽타주 길이나 재장전 시간이 비정상이면 기본 배속(1.0)으로
	float PlayRate = 1.0f;
	if (MontageLength > 0.f && ActualReloadTime > 0.f)
	{
		PlayRate = MontageLength / ActualReloadTime;
	}

	PlayAnimMontage(ReloadMontage, PlayRate);
}

void ARGCharacter::StopReloadAnimation()
{
	if (ReloadMontage)
	{
		StopAnimMontage(ReloadMontage);
	}
}

float ARGCharacter::GetMoveSpeedMultiplier() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks =
				Progression->GetUpgradeStackCount(FName(TEXT("MoveSpeedUp")));

			const float EffectAmount =
				Progression->GetUpgradeEffectAmount(FName(TEXT("MoveSpeedUp")));

			return 1.0f + (EffectAmount * Stacks);
		}
	}

	return 1.0f;
}

float ARGCharacter::GetMaxHealthWithUpgrade() const
{
	float Result = MaxHealth;

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks =
				Progression->GetUpgradeStackCount(FName(TEXT("MaxHealthUp")));

			const float EffectAmount =
				Progression->GetUpgradeEffectAmount(FName(TEXT("MaxHealthUp")));

			Result += EffectAmount * Stacks;
		}
	}

	return FMath::RoundToFloat(Result);
}

float ARGCharacter::GetCurrentMaxHealth() const
{
	return GetMaxHealthWithUpgrade();
}

float ARGCharacter::GetRegenerationPerSecond() const
{
	float RegenPerSecond = MaxHealth * RegenerationMultipiler;

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression =
			GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks =
				Progression->GetUpgradeStackCount(FName(TEXT("RegenUp")));

			const float EffectAmount =
				Progression->GetUpgradeEffectAmount(FName(TEXT("RegenUp")));

			RegenPerSecond += EffectAmount * Stacks;
		}
	}

	return RegenPerSecond;
}