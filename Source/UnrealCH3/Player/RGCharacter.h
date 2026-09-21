#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RGCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UTimelineComponent;
class ARGBaseWeapon;
struct FInputActionValue;

// [추가] 무기가 실제로 생성/교체된 순간을 외부 시스템(UI 등)에 알린다.
// Character가 UIManager를 직접 참조하지 않도록 Delegate로 느슨하게 연결한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWeaponEquipped,
	ARGBaseWeapon*, NewWeapon
);

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
	Grappling,
	Falling
};

UCLASS()
class UNREALCH3_API ARGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARGCharacter();

	// [추가] 무기 장착 완료 이벤트. 초기 무기와 이후 무기 교체 모두 동일한 경로로 알린다.
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponEquipped OnWeaponEquipped;

protected:
	// override 함수
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DmageEvent, AController* EventIntigator, AActor* DamageCauser) override;
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

	// 벽타기
	UFUNCTION()
	void OnStartWallRun();
	UFUNCTION()
	void OnStopWallRun();

	// Grapple
	UFUNCTION()
	void OnCanGrapple();
	UFUNCTION()
	void OnStopGrapple();

	// 애니메이션
	UFUNCTION()
	void PlayFireAnimation();
	UFUNCTION()
	void StopFireAnimation();
	UFUNCTION()
	void PlayReloadAnimation();
	UFUNCTION()
	void StopReloadAnimation();
public:
	// 겟터
	UFUNCTION(BlueprintPure, Category = "State")
	EMovementState GetCurrentMovementState() const { return CurrentMovementState; }
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	ARGBaseWeapon* GetCurrentWeapon() const { return CurrentWeapon; }
	int32 GetSlopeType();

public:
	// 셋터
	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);
	UFUNCTION(BlueprintCallable)
	void SetCheckPoint();
	void SetAimState(bool bCanAim);
	void SetSprintState(bool bCanSprint);
	void SetMovementState(EMovementState NewState, bool bForce = false);

public:
	// Reset and Clear
	void ResetDashCount() { DashCount = MaxDashCount; }
	void ResetAllState();
	void ClearTimerHandle();

public:
	// 업데이트
	void ReturnCheckPoint();
	void CheckSlideSpeed();
	void StartRegenerateHealth();
	void TickRegenerateHealth();
	void Dead();

	// 무기 장착 (UI에서 무기선택시 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(TSubclassOf<ARGBaseWeapon> SpawnWeaponClass);

private:
	// 입력 함수들
	void Move(const FInputActionValue& value);
	void StopMove(const FInputActionValue& value);

	void Look(const FInputActionValue& value);

	void StartJump(const FInputActionValue& value);
	void StopJump(const FInputActionValue& value);

	void Dash(const FInputActionValue& value);

	void ToggleSprint(const FInputActionValue& value);

	void ToggleCrouch(const FInputActionValue& value);
	void StartCrouch();
	void StopCrouch();

	void StartSliding();
	void StopSliding();

	void StartGrapple(const FInputActionValue& value);

	void StartFire(const FInputActionValue& value);
	void StopFire(const FInputActionValue& value);

	void StartAim(const FInputActionValue& value);
	void StopAim(const FInputActionValue& value);

	void Reload(const FInputActionValue& value);

private:
	// 카메라
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// WallRun 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<class URGWallRunMovement> WallRunMovement;
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<class URGGrappleComponent> GrappleComponent;

	// 무기
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ARGBaseWeapon> CurrentWeapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ARGBaseWeapon> WeaponClass;

private:
	// 애니메이션
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> FireMontage;
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	// 상태 변수
	UPROPERTY(VisibleAnywhere, Category = "State|Movement")
	EMovementState CurrentMovementState = EMovementState::Idle;
	bool bIsWallRunning = false;
	bool bIsSprinting = false;
	bool bIsAiming = false;
	bool bIsDead = false;
	bool bIsGodMode = false;

	// 에임 변수
	UPROPERTY(VisibleAnywhere, Category = "Aim")
	TObjectPtr<UTimelineComponent> AimTimeline;
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	TObjectPtr<UCurveFloat> AimCurve;
	UPROPERTY(EditAnywhere, Category = "Aim")
	float DefaultFOV = 90.0f;

	// Crouch 변수
	UPROPERTY(VisibleAnywhere, Category = "Crouch")
	TObjectPtr<UTimelineComponent> CrouchTimeline;
	UPROPERTY(EditDefaultsOnly, Category = "Crouch")
	TObjectPtr<UCurveFloat> CrouchCurve;
	UPROPERTY(EditAnywhere, Category = "Crouch")
	float CrouchCapsuleHeight = -48.0f;
	UPROPERTY(EditAnywhere, Category = "Crouch")
	FVector MeshRelativeLocation = FVector::ZeroVector;

	// 대쉬 변수
	UPROPERTY(VisibleAnywhere, Category = "Movement|Dash")
	TObjectPtr<UTimelineComponent> DashTimeline;
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	TObjectPtr<UCurveFloat> DashCurve;
	UPROPERTY(EditAnywhere, Category = "Movement|Dash", meta = (ClampMin = "100.0"))
	float DashDistance = 1000.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Dash", meta = (ClampMin = "1.0"))
	float DashCooldown = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Dash", meta = (ClampMin = "1"))
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
	UPROPERTY(EditAnywhere, Category = "CheckPoint", meta = (ClampMin = "0.1"))
	float CheckPointInterval = 20.0f;
	FVector LevelStartLocation = FVector::ZeroVector;
	FVector CheckPointLocation = FVector::ZeroVector;

	// 무적상태
	FTimerHandle GodModeTimerHandle;
	UPROPERTY(EditAnywhere, Category = "GodMode", meta = (ClampMin = "1.0"))
	float GodModeDuration = 1.0f;

	// Health 자동회복
	FTimerHandle StartRegenerationTimerHandle;
	FTimerHandle TickRegenerationTimerHandle;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "0.1"))
	float StartRegenerationDelay = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "0.1"))
	float TickRegenerationInterval = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "0.1"))
	float RegenerationMultipiler = 0.1f;

private:
	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
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

public:
	//강화용 함수
	float GetMoveSpeedMultiplier() const;
	float GetDefaultMoveSpeed() const;
	float GetSprintSpeed() const;
	float GetMaxHealthWithUpgrade() const;
	float GetCurrentMaxHealth() const;
	float GetRegenerationPerSecond() const;
	//팔 위아래 회전

};
