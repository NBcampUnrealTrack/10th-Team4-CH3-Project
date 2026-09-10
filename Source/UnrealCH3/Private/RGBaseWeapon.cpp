// Fill out your copyright notice in the Description page of Project Settings.

#include "RGBaseWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


ARGBaseWeapon::ARGBaseWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("WeaponMesh")
		);

	RootComponent = WeaponMesh;
}


void ARGBaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 데이터테이블에서 이 무기의 행(RowName)을 찾아 스탯을 캐싱
	if (WeaponStatsTable && !WeaponRowName.IsNone())
	{
		if (const FWeaponStatsRow* Row =
			WeaponStatsTable->FindRow<FWeaponStatsRow>(
				WeaponRowName,
				TEXT("WeaponStatsLookup")
			))
		{
			WeaponStats = *Row;
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT(
					"%s: WeaponStatsTable에서 RowName '%s'를 찾지 못했습니다. 기본값을 사용합니다."
				),
				*GetName(),
				*WeaponRowName.ToString()
			);
		}
	}

	CurrentAmmo = WeaponStats.MagazineCapacity;

	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		WeaponStats.MagazineCapacity
	);
}


void ARGBaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


// =========================================================
// Owner
// =========================================================

void ARGBaseWeapon::SetOwningCharacter(
	ACharacter* NewOwner
)
{
	OwningCharacter = NewOwner;

	SetOwner(NewOwner);
}


// =========================================================
// [P0] 공통 무기 / 발사
// =========================================================

bool ARGBaseWeapon::CanFire() const
{
	if (!bExternalActionsAllowed)
	{
		return false;
	}

	if (bIsReloading)
	{
		return false;
	}

	if (!HasAmmo())
	{
		return false;
	}

	return true;
}


void ARGBaseWeapon::StartFire()
{
	if (bWantsToFire)
	{
		return;
	}

	bWantsToFire = true;

	StartFireTimer();
}


void ARGBaseWeapon::StopFire()
{
	bWantsToFire = false;

	StopFireTimer();
}


void ARGBaseWeapon::StartFireTimer()
{
	if (!bExternalActionsAllowed)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(
		FireTimerHandle
	))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&ARGBaseWeapon::HandleFireTick,
		FMath::Max(
			0.01f,
			WeaponStats.FireInterval
		),
		true,
		0.0f
	);
}


void ARGBaseWeapon::StopFireTimer()
{
	GetWorldTimerManager().ClearTimer(
		FireTimerHandle
	);
}


void ARGBaseWeapon::HandleFireTick()
{
	if (!bWantsToFire || !CanFire())
	{
		StopFireTimer();

		return;
	}

	Fire();

	CurrentAmmo =
		FMath::Max(
			0,
			CurrentAmmo - 1
		);

	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		WeaponStats.MagazineCapacity
	);

	OnShotFired.Broadcast();

	if (CurrentAmmo <= 0)
	{
		bWantsToFire = false;

		StopFireTimer();

		StartReloaded();
	}
}


bool ARGBaseWeapon::GetMuzzleAimTransform(
	FVector& OutStart,
	FVector& OutDirection
) const
{
	if (!OwningCharacter)
	{
		return false;
	}

	FRotator ViewRotation;

	OwningCharacter->GetActorEyesViewPoint(
		OutStart,
		ViewRotation
	);

	OutDirection = ViewRotation.Vector();

	return true;
}


FVector ARGBaseWeapon::ApplySpread(
	const FVector& AimDirection
) const
{
	const float SpreadDegrees =
		bIsAiming
		? WeaponStats.AimSpread
		: WeaponStats.HipFireSpread;

	if (SpreadDegrees <= 0.0f)
	{
		return AimDirection;
	}

	const float SpreadRadians =
		FMath::DegreesToRadians(
			SpreadDegrees
		);

	return FMath::VRandCone(
		AimDirection,
		SpreadRadians
	);
}


void ARGBaseWeapon::Fire()
{
	FVector StartLocation;
	FVector FireDirection;

	if (!GetMuzzleAimTransform(
		StartLocation,
		FireDirection
	))
	{
		return;
	}

	const FVector SpreadDirection =
		ApplySpread(
			FireDirection
		);

	FireHitscan(
		StartLocation,
		SpreadDirection,
		-1.0f,
		nullptr
	);
}


bool ARGBaseWeapon::FireHitscan(
	const FVector& StartLocation,
	const FVector& FireDirection,
	float DamageOverride,
	TSet<AActor*>* AlreadyHitActors
)
{
	const FVector EndLocation =
		StartLocation +
		FireDirection * TraceRange;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(WeaponFire),
		true
	);

	QueryParams.AddIgnoredActor(this);

	if (OwningCharacter)
	{
		QueryParams.AddIgnoredActor(
			OwningCharacter
		);
	}

	FHitResult Hit;

	const bool bHit =
		GetWorld()->LineTraceSingleByChannel(
			Hit,
			StartLocation,
			EndLocation,
			TraceChannel,
			QueryParams
		);

	if (!bHit || !Hit.GetActor())
	{
		return false;
	}


	// 관통 무기에서 같은 대상 중복 처리 방지
	if (AlreadyHitActors)
	{
		if (AlreadyHitActors->Contains(
			Hit.GetActor()
		))
		{
			return false;
		}

		AlreadyHitActors->Add(
			Hit.GetActor()
		);
	}


	const float BaseDamage =
		(DamageOverride >= 0.0f)
		? DamageOverride
		: WeaponStats.BaseDamage;


	const bool bIsDirectHit =
		(AlreadyHitActors == nullptr) ||
		(AlreadyHitActors->Num() == 1);


	ApplyHitDamage(
		Hit,
		BaseDamage,
		StartLocation,
		bIsDirectHit
	);

	return true;
}


// =========================================================
// [P0] 홀드 조준
// =========================================================

bool ARGBaseWeapon::IsAiming() const
{
	return bIsAiming;
}


void ARGBaseWeapon::StartAiming()
{
	if (
		bIsReloading ||
		bIsAiming ||
		!bExternalActionsAllowed
		)
	{
		return;
	}

	bIsAiming = true;

	OnAimingChanged.Broadcast(true);
}


void ARGBaseWeapon::StopAiming()
{
	if (!bIsAiming)
	{
		return;
	}

	bIsAiming = false;

	OnAimingChanged.Broadcast(false);
}


// =========================================================
// [P0] 탄창 / 재장전
// =========================================================

bool ARGBaseWeapon::IsReloading() const
{
	return bIsReloading;
}


bool ARGBaseWeapon::CanReloaded() const
{
	if (!bExternalActionsAllowed)
	{
		return false;
	}

	if (bIsReloading)
	{
		return false;
	}

	if (
		CurrentAmmo >=
		WeaponStats.MagazineCapacity
		)
	{
		return false;
	}

	return true;
}


void ARGBaseWeapon::StartReloaded()
{
	if (!CanReloaded())
	{
		return;
	}

	StopAiming();
	StopFire();

	bIsReloading = true;

	OnReloadStarted.Broadcast();

	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&ARGBaseWeapon::OnReloadTimerComplete,
		FMath::Max(
			0.01f,
			WeaponStats.ReloadTime
		),
		false
	);
}


void ARGBaseWeapon::CancelReloaded()
{
	if (!bIsReloading)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(
		ReloadTimerHandle
	);

	bIsReloading = false;

	OnReloadCanceled.Broadcast();
}


void ARGBaseWeapon::OnReloadTimerComplete()
{
	CompleteReload();
}


void ARGBaseWeapon::CompleteReload()
{
	if (!bIsReloading)
	{
		return;
	}

	bIsReloading = false;


	if (WeaponStats.bReloadWholeMagazine)
	{
		CurrentAmmo =
			WeaponStats.MagazineCapacity;
	}
	else
	{
		CurrentAmmo =
			FMath::Min(
				WeaponStats.MagazineCapacity,
				CurrentAmmo + 1
			);
	}


	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		WeaponStats.MagazineCapacity
	);

	OnReloadCompleted.Broadcast();


	// 샷건처럼 한 발씩 장전하는 무기
	if (
		!WeaponStats.bReloadWholeMagazine &&
		CurrentAmmo <
		WeaponStats.MagazineCapacity &&
		bExternalActionsAllowed
		)
	{
		StartReloaded();
	}
}


// =========================================================
// Reload Progress
// =========================================================

float ARGBaseWeapon::GetReloadProgress() const
{
	if (!bIsReloading)
	{
		return 0.0f;
	}

	if (!GetWorld())
	{
		return 0.0f;
	}

	const float ReloadDuration =
		FMath::Max(
			0.01f,
			WeaponStats.ReloadTime
		);

	const float RemainingTime =
		GetWorldTimerManager()
		.GetTimerRemaining(
			ReloadTimerHandle
		);

	if (RemainingTime < 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(
		1.0f -
		(RemainingTime / ReloadDuration),
		0.0f,
		1.0f
	);
}


// =========================================================
// 외부 강제 취소 / 상태 잠금
// =========================================================

void ARGBaseWeapon::ForceCancelAllActions()
{
	StopFire();
	CancelReloaded();
	StopAiming();
}


void ARGBaseWeapon::SetExternalActionsAllowed(
	bool bAllowed
)
{
	bExternalActionsAllowed = bAllowed;

	if (!bAllowed)
	{
		ForceCancelAllActions();
	}
}


// =========================================================
// [P0] 공통 피해
// =========================================================

float ARGBaseWeapon::GetUpgradeDamageMultiplier() const
{
	// 강화 시스템 연결 전 기본 배율
	return 1.0f;
}


float ARGBaseWeapon::CalculateDistanceFalloffMultiplier(
	float Distance
) const
{
	if (
		WeaponStats.DamageFalloffStart <= 0.0f &&
		WeaponStats.DamageFalloffEnd <= 0.0f
		)
	{
		return 1.0f;
	}


	if (
		Distance <=
		WeaponStats.DamageFalloffStart
		)
	{
		return 1.0f;
	}


	if (
		Distance >=
		WeaponStats.DamageFalloffEnd
		)
	{
		return WeaponStats.MinFalloffDamageMultiplier;
	}


	const float Range =
		FMath::Max(
			1.0f,
			WeaponStats.DamageFalloffEnd -
			WeaponStats.DamageFalloffStart
		);


	const float Alpha =
		(
			Distance -
			WeaponStats.DamageFalloffStart
			) / Range;


	return FMath::Lerp(
		1.0f,
		WeaponStats.MinFalloffDamageMultiplier,
		Alpha
	);
}


void ARGBaseWeapon::ApplyHitDamage(
	const FHitResult& Hit,
	float BaseDamage,
	const FVector& ShotStart,
	bool bIsDirectHit
)
{
	AActor* HitActor =
		Hit.GetActor();

	if (!HitActor)
	{
		return;
	}


	// =====================================================
	// 1. 기본 피해
	// =====================================================

	float FinalDamage = BaseDamage;


	// =====================================================
	// 2. 강화 배율
	// =====================================================

	FinalDamage *=
		GetUpgradeDamageMultiplier();


	// =====================================================
	// 3. 거리 감쇠
	// =====================================================

	const float Distance =
		FVector::Dist(
			ShotStart,
			Hit.ImpactPoint
		);

	FinalDamage *=
		CalculateDistanceFalloffMultiplier(
			Distance
		);


	// =====================================================
	// 4. 약점
	// =====================================================

	const bool bIsWeakSpot =
		Hit.Component.IsValid() &&
		Hit.Component->ComponentHasTag(
			WeakSpotTag
		);

	if (bIsWeakSpot)
	{
		FinalDamage *=
			WeakSpotDamageMultiplier;
	}


	// =====================================================
	// 5. Damage Type
	// =====================================================

	TSubclassOf<UDamageType> DamageTypeClass =
		bIsDirectHit
		? URGDirectHitDamageType::StaticClass()
		: UDamageType::StaticClass();


	// =====================================================
	// 6. Unreal Damage 전달
	// =====================================================

	UGameplayStatics::ApplyPointDamage(
		HitActor,
		FinalDamage,
		(
			Hit.ImpactPoint -
			ShotStart
			).GetSafeNormal(),
		Hit,
		OwningCharacter
		? OwningCharacter->GetController()
		: nullptr,
		this,
		DamageTypeClass
	);


	// =====================================================
	// 7. UI / 외부 시스템에 Hit 정보 전달
	// =====================================================

	OnWeaponHit.Broadcast(
		HitActor,
		FinalDamage,
		bIsWeakSpot,
		Hit.ImpactPoint
	);
}


// =========================================================
// Damage Feedback Receiver
// =========================================================

void ARGBaseWeapon::ReceiveDamageFeedback(
	float AppliedDamage,
	bool bKilled,
	AActor* TargetActor,
	const FVector& WorldLocation
)
{
	if (AppliedDamage <= 0.0f)
	{
		return;
	}


	// Hit / Kill marker용
	OnDamageConfirmed.Broadcast(
		AppliedDamage,
		bKilled
	);


	// Damage Number용
	OnDamageNumberRequested.Broadcast(
		AppliedDamage,
		TargetActor,
		WorldLocation
	);
}