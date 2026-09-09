#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RGCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UTimelineComponent;
struct FInputActionValue;

// 캐릭터 이동상태 ENUM
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Waking,
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
	// override 함수
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DmageEvent, AController* EventIntigator, AActor* DamageCauser) override;

public:
	// Dash 타임라인 함수
	UFUNCTION()
	void OnDashUpdate(float Alpha);
	UFUNCTION()
	void OnDashFinished();

public:
	// 겟터
	UFUNCTION(BlueprintPure, Category = "State")
	EMovementState GetCurrentMovementState() const { return CurrentMovementState; }
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return CurrentHealth; }

	int32 GetSlopeType();

public:
	// 셋터
	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);
	UFUNCTION(BlueprintCallable)
	void SetCheckPoint();
	void SetAimState(bool bCanAim);
	void SetSprintState(bool bCanSprint);
	void StartRegenerateHealth();
	void TickRegenerateHealth();

public:
	// Reset and Clear
	void ResetDashCount() { DashCount = MaxDashCount; }
	void ClearTimerHandle();
	void ResetAllState();

public:
	// 상태 업데이트
	void UpdateMovementState();
	void ReturnCheckPoint();
	void Dead();

private:
	// 입력 함수들
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void StartJump(const FInputActionValue& value);
	void StopJump(const FInputActionValue& value);
	void Dash(const FInputActionValue& value);
	void ToggleSprint(const FInputActionValue& value);
	void StartCrouch(const FInputActionValue& value);
	void StopCrouch(const FInputActionValue& value);
	void Shoot(const FInputActionValue& value);
	void ToggleAim(const FInputActionValue& value);
	void Reload(const FInputActionValue& value);

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
	// 상태 변수
	UPROPERTY(VisibleAnywhere, Category = "State|Movement")
	EMovementState CurrentMovementState = EMovementState::Idle;
	bool bIsWallRunning = false;
	bool bIsSprinting = false;
	bool bIsAiming = false;
	bool bIsDead = false;
	bool bIsGodMode = false;

	// 대쉬 변수
	UPROPERTY(VisibleAnywhere, Category = "Timeline|Dash")
	TObjectPtr<UTimelineComponent> DashTimeline;
	UPROPERTY(EditDefaultsOnly, Category = "Timeline|Dash", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCurveFloat> DashCurve;
	UPROPERTY(EditAnywhere, Category = "Timeline|Dash", meta = (AllowPrivateAccess = "true"))
	float DashDistance = 1000.0f;
	UPROPERTY(EditAnywhere, Category = "Timeline|Dash", meta = (AllowPrivateAccess = "true"))
	float DashCooldown = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Timeline|Dash", meta = (AllowPrivateAccess = "true"))
	int32 MaxDashCount = 1;
	int32 DashCount = MaxDashCount;

	FTimerHandle DashCooldownTimerHandle;
	FVector DashStartLocation = FVector::ZeroVector;
	FVector DashEndLocation = FVector::ZeroVector;
	FVector DashVelocity = FVector::ZeroVector;
	FVector2D MoveInput = FVector2D::ZeroVector;
	FHitResult DashHitResult;

	// 슬라이딩 변수
	float GroundFriction = 0.0f;
	

	// 체크포인트
	FTimerHandle CheckPointTimerHandle;
	UPROPERTY(EditDefaultsOnly, Category = "CheckPoint", meta = (AllowPrivateAccess = "true"))
	float CheckPointInterval = 20.0f;
	FVector LevelStartLocation = FVector::ZeroVector;
	FVector CheckPointLocation = FVector::ZeroVector;

	// 무적상태
	FTimerHandle GodModeTimerHandle;
	UPROPERTY(EditDefaultsOnly, Category = "GodMode", meta = (AllowPrivateAccess = "true"))
	float GodModeDuration = 1.0f;

	// Health 자동회복
	FTimerHandle StartRegenerationTimerHandle;
	FTimerHandle TickRegenerationTimerHandle;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float StartRegenerationDelay = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float TickRegenerationInterval = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float RegenerationMultipiler = 0.1f;

private:
	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = MaxHealth;

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultSpeed = 650.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 950.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultAccelration = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultJumpZVelocity = 700.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultAirControl = 0.45;

	float DefaultGroundFriction = 0.0f;
	float DefaultBreakingDecelerationWalking = 0.0f;
};
