#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGWeaponStats.h"
#include "Combat/DamageFeedbackReceiver.h"
#include "RGBaseWeapon.generated.h"

class USkeletalMeshComponent;
class UDataTable;
class ACharacter;


// =========================================================
// UI 바인딩용 델리게이트
// =========================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAmmoChanged,
	int32, CurrentAmmo,
	int32, MagazineCapacity
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnAimingChanged,
	bool, bNowAiming
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadCanceled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponShotFired);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnWeaponDamageConfirmed,
	float, AppliedDamage,
	bool, bKilled
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnWeaponDamageNumberRequested,
	float, AppliedDamage,
	AActor*, TargetActor,
	FVector, WorldLocation
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnWeaponHit,
	AActor*, HitActor,
	float, FinalDamage,
	bool, bIsWeakspot,
	FVector, HitLocation
);


// =========================================================
// 직접 공격용 DamageType
// =========================================================

UCLASS()
class UNREALCH3_API URGDirectHitDamageType : public UDamageType
{
	GENERATED_BODY()
};


// =========================================================
// Base Weapon
// =========================================================

UCLASS()
class UNREALCH3_API ARGBaseWeapon
	: public AActor
	, public IDamageFeedbackReceiver
{
	GENERATED_BODY()

public:

	ARGBaseWeapon();

	virtual void Tick(float DeltaTime) override;


	// =========================================================
	// UI / Weapon Events
	// =========================================================

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadStarted OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadCompleted OnReloadCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAimingChanged OnAimingChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadCanceled OnReloadCanceled;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponShotFired OnShotFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponHit OnWeaponHit;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponDamageConfirmed OnDamageConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponDamageNumberRequested OnDamageNumberRequested;


	// =========================================================
	// Debug
	// =========================================================

	UPROPERTY(EditAnywhere, Category = "Weapon|Debug")
	bool bShowWeaponTraceDebug = false;


	// =========================================================
	// Damage Feedback Interface
	// =========================================================

	virtual void ReceiveDamageFeedback(
		float AppliedDamage,
		bool bKilled,
		AActor* TargetActor,
		const FVector& WorldLocation
	) override;


protected:

	virtual void BeginPlay() override;


	// =========================================================
	// Fire
	// =========================================================

	virtual void Fire();

	virtual bool FireHitscan(
		const FVector& StartLocation,
		const FVector& FireDirection,
		float DamageOverride,
		TSet<AActor*>* AlreadyHitActors
	);

	virtual void ApplyHitDamage(
		const FHitResult& Hit,
		float BaseDamage,
		const FVector& ShotStart,
		bool bIsDirectHit
	);

	virtual bool GetMuzzleAimTransform(
		FVector& OutStart,
		FVector& OutDirection
	) const;

	virtual void CompleteReload();

	virtual float GetUpgradeDamageMultiplier() const;

	virtual float CalculateDistanceFalloffMultiplier(
		float Distance
	) const;

	virtual FVector ApplySpread(
		const FVector& AimDirection
	) const;


	// =========================================================
	// Damage
	// =========================================================

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	FName WeakSpotTag = FName(TEXT("Weakspot"));

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	float WeakSpotDamageMultiplier = 1.5f;


	// =========================================================
	// Components
	// =========================================================

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Weapon"
	)
	USkeletalMeshComponent* WeaponMesh;


	// =========================================================
	// Weapon Stats
	// =========================================================

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	UDataTable* WeaponStatsTable;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	FName WeaponRowName;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FWeaponStatsRow WeaponStats;


	// =========================================================
	// Trace
	// =========================================================

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Trace")
	float TraceRange = 10000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;


	// =========================================================
	// Runtime State
	// =========================================================

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bWantsToFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bExternalActionsAllowed = true;


	ACharacter* OwningCharacter = nullptr;


	// =========================================================
	// Timers
	// =========================================================

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;


private:

	UFUNCTION()
	void OnReloadTimerComplete();

	void HandleFireTick();

	void StartFireTimer();

	void StopFireTimer();

	bool HasAmmo() const
	{
		return CurrentAmmo > 0;
	}


public:

	// =========================================================
	// Character / Input Interface
	// =========================================================

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartAiming();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopAiming();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartReloaded();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void CancelReloaded();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void ForceCancelAllActions();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetExternalActionsAllowed(bool bAllowed);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetOwningCharacter(ACharacter* NewOwner);


	// =========================================================
	// State Getters
	// =========================================================

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanReloaded() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAiming() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReloading() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const
	{
		return CurrentAmmo;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMagazineCapacity() const
	{
		return WeaponStats.MagazineCapacity;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetADSFOV() const
	{
		return WeaponStats.ADSFOVMultiplier;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadProgress() const;
};