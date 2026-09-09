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
	Dashing,
	WallRunning,
	Falling
};

UCLASS()
class UNREALCH3_API ARGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARGCharacter();

	// override 함수
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DmageEvent, AController* EventIntigator, AActor* DamageCauser) override;

	// Dash 타임라인 함수
public:
	UFUNCTION()
	void OnDashUpdate(float Alpha);
	UFUNCTION()
	void OnDashFinished();

	// 겟터
public:
	UFUNCTION(BlueprintCallable, Category = "State")
	EMovementState GetCurrentMovementState() const { return CurrentMovementState; }
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealth() const { return CurrentHealth; }

	// 셋터
public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);
	UFUNCTION()
	void SetCheckPoint();

	void ResetDashCount() { DashCount = MaxDashCount; }

	// 상태 업데이트
public:
	void UpdateMovementState();
	void ComebackCheckPoint();

	// 입력 함수들
private:
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void StartJump(const FInputActionValue& value);
	void StopJump(const FInputActionValue& value);
	void Dash(const FInputActionValue& value);
	void Shoot(const FInputActionValue& value);
	void ToggleAim(const FInputActionValue& value);
	void Reload(const FInputActionValue& value);

	// 상태 변수
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EMovementState CurrentMovementState = EMovementState::Idle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsWallRunning = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// 컴포넌트
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// 대쉬 변수
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timeline|Dash")
	TObjectPtr<UTimelineComponent> DashTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timeline|Dash")
	TObjectPtr<UCurveFloat> DashCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline|Dash")
	float DashDistance = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline|Dash")
	float DashCooldown = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline|Dash")
	int32 MaxDashCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline|Dash")
	int32 DashCount = MaxDashCount;

private:
	FTimerHandle DashCooldownTimerHandle;
	FVector DashStartLocation;
	FVector DashEndLocation;
	FVector2D MoveInput;
	FHitResult DashHitResult;

	FTimerHandle CheckPointTimerHandle;
	float CheckPointInterval = 20.0f;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = MaxHealth;

	FVector CheckPoint;
	bool bIsAiming = false;
};
