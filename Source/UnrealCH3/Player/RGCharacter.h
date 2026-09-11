#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RGCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UTimelineComponent;
class ARGBaseWeapon;
struct FInputActionValue;

// 캐릭터 이동상태 ENUM
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Walking,
	Sprinting,
	Dashing,
	Sliding,
	WallRunning,
	Falling
};

UCLASS()
class UNREALCH3_API ARGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARGCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DmageEvent,
		AController* EventIntigator,
		AActor* DamageCauser
	) override;

	virtual void Falling() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual bool CanJumpInternal_Implementation() const override;

public:
	// Dash 타임라인
	UFUNCTION()
	void OnDashUpdate(float Alpha);

	UFUNCTION()
	void OnDashFinished();

	// Aim 타임라인
	UFUNCTION()
	void OnAimUpdate(float Alpha);

	// Crouch 카메라 타임라인
	UFUNCTION()
	void OnCrouchCameraUpdate(float Alpha);

public:
	// Getter
	UFUNCTION(BlueprintPure, Category = "State")
	EMovementState GetCurrentMovementState() const
	{
		return CurrentMovementState;
	}

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const
	{
		return CurrentHealth;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	ARGBaseWeapon* GetCurrentWeapon() const
	{
		return CurrentWeapon;
	}

	int32 GetSlopeType();

public:
	// Setter
	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);

	UFUNCTION(BlueprintCallable)
	void SetCheckPoint();

	void SetAimState(bool bCanAim);
	void SetSprintState(bool bCanSprint);

public:
	// Reset and Clear
	void ResetDashCount()
	{
		DashCount = MaxDashCount;
	}

	void ResetAllState();
	void ClearTimerHandle();

public:
	// 업데이트
	void ReturnCheckPoint();
	void CheckSlideSpeed();
	void StartRegenerateHealth();
	void TickRegenerateHealth();
	void Dead();

	// 무기 장착
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(TSubclassOf<ARGBaseWeapon> SpawnWeaponClass);

private:
	// 입력
	void Move(const FInputActionValue& Value);
	void StopMove(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void ToggleSprint(const FInputActionValue& Value);
	void ToggleCrouch(const FInputActionValue& Value);
	void StartFire(const FInputActionValue& Value);
	void StopFire(const FInputActionValue& Value);
	void StartAim(const FInputActionValue& Value);
	void StopAim(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);

	// 슬라이딩
	void StartSliding();
	void StopSliding();

	// 벽달리기
	void StartWallRun();
	void StopWallRun();

private:
	// 카메라
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

private:
	// 무기
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Weapon",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<ARGBaseWeapon> CurrentWeapon;

	// Blueprint에서 BP_RGAssaultRifle 지정 가능
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Weapon",
		meta = (AllowPrivateAccess = "true")
	)
	TSubclassOf<ARGBaseWeapon> WeaponClass;

private:
	// 상태 변수
	UPROPERTY(VisibleAnywhere, Category = "State|Movement")
	EMovementState CurrentMovementState = EMovementState::Idle;

	bool bIsWallRunning = false;
	bool bIsSprinting = false;
	bool bIsAiming = false;
	bool bIsDead = false;
	bool bIsGodMode = false;

	// Aim
	UPROPERTY(VisibleAnywhere, Category = "Aim")
	TObjectPtr<UTimelineComponent> AimTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	TObjectPtr<UCurveFloat> AimCurve;

	UPROPERTY(EditAnywhere, Category = "Aim")
	float DefaultFOV = 90.0f;

	// Crouch
	UPROPERTY(VisibleAnywhere, Category = "Crouch")
	TObjectPtr<UTimelineComponent> CrouchTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Crouch")
	TObjectPtr<UCurveFloat> CrouchCurve;

	UPROPERTY(EditAnywhere, Category = "Crouch")
	float CrouchCapsuleValue = -44.0f;

	// Dash
	UPROPERTY(VisibleAnywhere, Category = "Movement|Dash")
	TObjectPtr<UTimelineComponent> DashTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	TObjectPtr<UCurveFloat> DashCurve;

	UPROPERTY(
		EditAnywhere,
		Category = "Movement|Dash",
		meta = (ClampMin = "100.0")
	)
	float DashDistance = 1000.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Movement|Dash",
		meta = (ClampMin = "1.0")
	)
	float DashCooldown = 3.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Movement|Dash",
		meta = (ClampMin = "1")
	)
	int32 MaxDashCount = 1;

	int32 DashCount = MaxDashCount;

	FTimerHandle DashCooldownTimerHandle;

	FVector DashStartLocation = FVector::ZeroVector;
	FVector DashEndLocation = FVector::ZeroVector;
	FVector DashVelocity = FVector::ZeroVector;
	FVector2D MoveInput = FVector2D::ZeroVector;

	FHitResult DashHitResult;

	// 슬라이드
	FTimerHandle SlideTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float StopSlideSpeed = 50.0f;

	// 체크포인트
	FTimerHandle CheckPointTimerHandle;

	UPROPERTY(
		EditAnywhere,
		Category = "CheckPoint",
		meta = (ClampMin = "0.1")
	)
	float CheckPointInterval = 20.0f;

	FVector LevelStartLocation = FVector::ZeroVector;
	FVector CheckPointLocation = FVector::ZeroVector;

	// 무적
	FTimerHandle GodModeTimerHandle;

	UPROPERTY(
		EditAnywhere,
		Category = "GodMode",
		meta = (ClampMin = "1.0")
	)
	float GodModeDuration = 1.0f;

	// Health 자동회복
	FTimerHandle StartRegenerationTimerHandle;
	FTimerHandle TickRegenerationTimerHandle;

	UPROPERTY(
		EditAnywhere,
		Category = "Health",
		meta = (ClampMin = "0.1")
	)
	float StartRegenerationDelay = 5.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Health",
		meta = (ClampMin = "0.1")
	)
	float TickRegenerationInterval = 1.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Health",
		meta = (ClampMin = "0.1")
	)
	float RegenerationMultipiler = 0.1f;

private:
	// Health
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Health",
		meta = (AllowPrivateAccess = "true")
	)
	float MaxHealth = 100.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Health",
		meta = (AllowPrivateAccess = "true")
	)
	float CurrentHealth = MaxHealth;

	// Movement
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Movement",
		meta = (AllowPrivateAccess = "true")
	)
	float DefaultSpeed = 650.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Movement",
		meta = (AllowPrivateAccess = "true")
	)
	float SprintSpeed = 950.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Movement",
		meta = (AllowPrivateAccess = "true")
	)
	float DefaultAccelration = 3000.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Movement",
		meta = (AllowPrivateAccess = "true")
	)
	float DefaultJumpZVelocity = 700.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Movement",
		meta = (AllowPrivateAccess = "true")
	)
	float DefaultAirControl = 0.45f;

	float DefaultGroundFriction = 0.0f;
	float DefaultBreakingDecelerationWalking = 0.0f;
};