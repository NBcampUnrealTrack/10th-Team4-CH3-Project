// Fill out your copyright notice in the Description page of Project Settings.
#include "RGBaseWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Gamemode/RGProgressionSubsystem.h"
#include "DrawDebugHelpers.h"

ARGBaseWeapon::ARGBaseWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
}

void ARGBaseWeapon::ReceiveDamageFeedback(float AppliedDamage, bool bKilled, AActor* TargetActor, const FVector& WorldLocation)
{
	if (AppliedDamage <= 0.f)
	{
		return;
	}
	//히트,킬 마커
	OnDamageConfirmed.Broadcast(AppliedDamage, bKilled);

	//데미지 숫자
	OnDamageNumberRequested.Broadcast(AppliedDamage, TargetActor, WorldLocation);
}

void ARGBaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 데이터테이블에서 이 무기의 행(RowName)을 찾아 스탯을 캐싱합니다.
	if (WeaponStatsTable && !WeaponRowName.IsNone())
	{
		//WeaponStatsTable이라는 데이터 테이블에 들어가서 WeaponRowName에 해당하는 무기를 가져옴
		//ex BP_AssaultRifle의 디테일패널에 Weapon|Stats에 들어가서 AssaultRifle이라고 타이핑해두면 WeaponRowName에 AssaultRifle이 들어감
		//그리고 WeaponStats에는 AssaultRifle에 해당하는 기본 스탯들이 전부 들어간 행 그 자체가 되는 것. -> WeaponStats.~~~를 통해 무기 스탯을 불러올 수 있다.
		if (const FWeaponStatsRow* Row = WeaponStatsTable->FindRow<FWeaponStatsRow>(WeaponRowName, TEXT("WeaponStatsLookup")))
		{
			WeaponStats = *Row;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: WeaponStatsTable에서 RowName '%s'를 찾지 못했습니다. 기본값을 사용합니다."), *GetName(), *WeaponRowName.ToString());
		}
	}

	CurrentAmmo = GetMagazineCapacity();

	// 강화 즉시 반영을 위한 구독
	
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			LastKnownMagazineStack = Progression->GetUpgradeStackCount(FName(TEXT("MagazineUp")));
			Progression->OnUpgradeApplied.AddDynamic(this, &ARGBaseWeapon::HandleUpgradeApplied);
		}
	}
	
}

void ARGBaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// OwningCharacter에 실제로 무기를 들고 올 캐릭터를 등록
// 캐릭터의 조준 위치를 확인할때나 , 라인트레이스에서 누가 쐈는지 확인할 때 사용
void ARGBaseWeapon::SetOwningCharacter(ACharacter* NewOwner)
{
	OwningCharacter = NewOwner;
	SetOwner(NewOwner);
}

// [P0] 공통 무기·발사
// 
//발사 가능한지 확인
// =========== 발사 시스템 요약 ============
// StartFire에서StartFireTimer를 호출한다. -> StartFireTimer는 HandleFireTick함수를 FireInterval 간격에 따라서 반복한다.
//HandleFireTick 함수는 많은 것을 한다. 발사 가능한 상태인지 확인. 발사 . 탄약 줄이기 등의 역할을 한다.
//HandleFireTick에서 호출된 Fire()는 GetMuzzleAimTransform()를 통해 카메라의 위치와 보는 방향세팅하고
//ApplySpread()로 조준 상태에 맞는 원뿔 각도만큼 방향을 흩뜨린 뒤 FireHitscan()에 넘김

//바인딩 시 좌클릭 누를 때 StartFire() 호출 -> 땔 때 StopFire() 호출 해야함
bool ARGBaseWeapon::CanFire() const
{	//외부액션중이면 (강화 중 등)
	if (!bExternalActionsAllowed) return false;
	//리로드중이면
	if (bIsReloading) return false;
	//총알이없으면
	if (!HasAmmo()) return false;
	return true;
}

void ARGBaseWeapon::StartFire()
{
	//발사버튼이 눌려있으면 bWantsToFire true로 함 . 중복 발사 방지
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
	//FireTimerHandle이 동작하고있으면 그냥 리턴
	if (GetWorldTimerManager().IsTimerActive(FireTimerHandle))
	{
		return;
	}
	const float FireInterval = GetFireInterval();
	float InitialDelay = 0.f;
	if (GetWorld())
	{
		const float TimeSinceLastFire = GetWorld()->GetTimeSeconds() - LastFireTime;
		InitialDelay = FMath::Max(0.f, FireInterval - TimeSinceLastFire);
	}
	//FireTimerHandle = 이름표 / this = 무기 자신 / &ARGBaseWeapon::HandleFireTick = 호출할 함수 / 
	//FMath::Max(0.01f, WeaponStats.FireInterval) = 반복간격 / true -> 계속 반복  / 0.f = 눌렀을 때 지연시간 (0.f 이므로 누르자마자 나감)
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ARGBaseWeapon::HandleFireTick, FireInterval, true, InitialDelay);
}

void ARGBaseWeapon::StopFireTimer()
{
	OnShotFiredStop.Broadcast();
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}
//이 함수에 의해 틱(발사시간간격)마다 적용되는 것 -> 발사 가능한 상태인지 실시간 확인 , 탄약 줄이기 , 델리게이트 , 탄퍼짐 , 발사 , 라인트레이스
void ARGBaseWeapon::HandleFireTick()
{
	//발사 가능한 상태인가?
	if (!bWantsToFire || !CanFire())
	{
		StopFireTimer();
		return;
	}
	// 여기서 기록 — Fire()가 어떤 자식 클래스에서 오버라이드되든 항상 거쳐감
	//스팸 클릭 막기 위한 코드,
	if (GetWorld())
	{
		LastFireTime = GetWorld()->GetTimeSeconds();
	}

	//그렇다면 발사
	Fire();
	//탄약 줄이기 
	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	// 발사 처리 Broadcast
	OnShotFired.Broadcast();

	// 탄약이 바뀌는 이 시점에만 딱 한 번 방송. 누가 듣고 있는지는 몰라도 됨.
	OnAmmoChanged.Broadcast(CurrentAmmo, GetMagazineCapacity());
	//탄약이 0이라면 
	if (CurrentAmmo <= 0)
	{
		bWantsToFire = false;
		StopFireTimer();
		StartReloaded();
	}
}

bool ARGBaseWeapon::GetMuzzleAimTransform(FVector& OutStart, FVector& OutDirection) const
{
	if (!OwningCharacter)
	{
		return false;
	}
	FRotator ViewRotation;
	//캐릭터의 카메라 위치와 보고있는 방향을 얻어온다.
	OwningCharacter->GetActorEyesViewPoint(OutStart, ViewRotation);
	//OutStart와 OutDirection은 참조이기 때문에 이 함수는 받은 매개변수를 세팅하는 역할을 한다.
	//실제로 Fire에서 GetMuzzleAimTransform을 호출해서 FVector 값을 세팅함.
	OutDirection = ViewRotation.Vector();
	return true;
}

FVector ARGBaseWeapon::ApplySpread(const FVector& AimDirection) const
{
	const float SpreadDegrees = bIsAiming ? WeaponStats.AimSpread : WeaponStats.HipFireSpread;

	// 0도 이하이면 완벽한 직선(퍼짐 없음)이므로 굳이 랜덤 계산 없이 원래 방향 그대로 반환
	if (SpreadDegrees <= 0.f)
	{
		return AimDirection;
	}

	const float SpreadRadians = FMath::DegreesToRadians(SpreadDegrees);
	// AimDirection을 중심축으로 하는 원뿔 안에서 균일 분포로 방향을 하나 뽑음
	return FMath::VRandCone(AimDirection, SpreadRadians);
}

void ARGBaseWeapon::Fire()
{
	FVector StartLocation, FireDirection;
	if (!GetMuzzleAimTransform(StartLocation, FireDirection))
	{
		return;
	}

	// 조준 여부에 맞는 탄퍼짐을 적용해서 실제 발사 방향을 흩뜨림
	const FVector SpreadDirection = ApplySpread(FireDirection);

	// FireHitscan()에 StartLocation , (퍼짐 적용된) SpreadDirection 전달
	FireHitscan(StartLocation, SpreadDirection, -1.f, nullptr);
}

bool ARGBaseWeapon::FireHitscan(const FVector& StartLocation, const FVector& FireDirection, float DamageOverride, TSet<AActor*>* AlreadyHitActors , FHitResult* OutHit)
{	
	//광선의 끝 지점을 계산 
	const FVector EndLocation = StartLocation + FireDirection * TraceRange;
	//QueryParams라는 트레이스 검사 옵션을 담는 객체이다.
	//WeaponFire라는 이름으로 몇 번 걸렸는지 통계를 냄.

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponFire), false);
	//무기 자기 자신은 맞은걸로 안침
	QueryParams.AddIgnoredActor(this);
	//캐릭터 자기 자신은 맞은걸로 안침
	if (OwningCharacter)
	{
		QueryParams.AddIgnoredActor(OwningCharacter);
	}

	//이미 맞은 적들을 라인 트레이스가 감지하지 않도록 무시목록에 넣는다
	//레일건 전용
	if (AlreadyHitActors)
	{
		for (AActor* AlreadyHitActor : *AlreadyHitActors)
		{
			QueryParams.AddIgnoredActor(AlreadyHitActor);
		}
	}

	//트레이스 결과를 담을 그릇.
	//라인트레이스에 들어가서 맞은 적들 다 Hit에 집어넣어버리고 Hit 안에 있는 엑터들에 대미지 줄 예정
	FHitResult Hit;
	//Hit -> 맞은 녀석들을 Hit에 넣을 것이다. / StartLocation -> 라인트레이스 시작 / EndLocation -> 라인트레이스 끝지점 /
	// TraceChannel -> 라인트레이스에 걸릴 녀석들의 채널 종류 ex)TraceChannel = ECC_Visibility 하면 Visibility 채널에 있는 녀석들만 걸림. / 
	//그래서 맞았으면 true 아니면 false
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, TraceChannel, QueryParams);

	if (!bHit || !Hit.GetActor())
	{
		return false;
	}

	const float BaseDamage = (DamageOverride >= 0.f) ? DamageOverride : WeaponStats.BaseDamage;

	if (AlreadyHitActors)
	{
		AlreadyHitActors->Add(Hit.GetActor());
	}

	// "직격"의 기준은 무기 종류가 아니라 관통 순서임.
	// AlreadyHitActors가 없으면(nullptr) 애초에 관통을 아예 안 쓰는 무기 -> 항상 직격.
	// AlreadyHitActors가 있으면(관통 무기) -> 이 트레이스에서 "처음" 맞은 대상일 때만 직격으로 인정.
	//   (Add는 이 아래 if문에서 이미 실행됐으므로, 여기서는 "방금 추가되기 전엔 비어있었는지"를 따로 셈)
	const bool bIsDirectHit = (AlreadyHitActors == nullptr) || (AlreadyHitActors->Num() == 1);

	ApplyHitDamage(Hit, BaseDamage, StartLocation, bIsDirectHit);
	if (OutHit) {
		*OutHit = Hit;
	}
	return true;
}

// ============================================================================
// [P0] 홀드 조준
// ============================================================================
// 조준 기능 주의할 것은 , 조준 중이다. 아니다 , 현재 에이밍이 가능한 상태다 등의 상태 정보만 전달하는 역할이다.
// 델리게이트도 전달함.
// 무기 별 조준 배율은 RGWeaponStats.h 의 FWeaponStatsRow 구조체에 ADSFOVMultiplier 변수로 존재함

bool ARGBaseWeapon::IsAiming() const
{
	return bIsAiming;
}

void ARGBaseWeapon::StartAiming()
{
	if (bIsReloading || bIsAiming || !bExternalActionsAllowed)
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

// ============================================================================
// [P0] 탄창·재장전
// ============================================================================

//주의 여기서의 재장전 방식은 인벤토리에 들어있는 전체 탄창을 고려하지 않았음
//보유 탄창이 무한하고 탄창에 따른 제약없이 재장전이 된다고 가정하고 만든 재장전.

bool ARGBaseWeapon::IsReloading() const
{
	return bIsReloading;
}

//UI 담당자 추가 함수 (재장전 진행률 반환 함수)
float ARGBaseWeapon::GetReloadProgress() const
{
	UWorld* World = GetWorld();

	if (!bIsReloading || !World)
	{
		return 0.0f;
	}

	const FTimerManager& TimerManager = World->GetTimerManager();

	//타이머 지속시간 
	const float Duration =
		TimerManager.GetTimerRate(ReloadTimerHandle);

	//타이머 경과시간
	const float Elapsed =
		TimerManager.GetTimerElapsed(ReloadTimerHandle);

	//타이머 유효성 검사
	if (Duration <= 0.f || Elapsed < 0.f)
	{
		return 0.f;
	}

	// 재장전 진행률 계산 경과시간 / 지속시간
	return FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
}

bool ARGBaseWeapon::CanReloaded() const
{	//리로드 될 수 있는지 확인.
	//조건은 외부 창이 동작하고있는가 -> 이미 리로딩중인가 -> 지금 총알이 풀인가. 전부 아니여야 true 반환
	if (!bExternalActionsAllowed) return false;
	if (bIsReloading) return false;
	if (CurrentAmmo >= GetMagazineCapacity()) return false;
	return true;
}

//UI 담당자 변경 타이머 -> 알림 순으로 함수 변경
void ARGBaseWeapon::StartReloaded()
{
	if (!CanReloaded())
	{
		return;
	}
	//재장전 시 조준 , 발사 스톱
	StopAiming();
	StopFire();

	bIsReloading = true;

	// 재장전을 시작할 때 현재 최대 탄창을 저장
	ReloadTargetMagazine = GetMagazineCapacity();

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ARGBaseWeapon::OnReloadTimerComplete, GetReloadTime(), false);
	
	OnReloadStarted.Broadcast();	
}

void ARGBaseWeapon::CancelReloaded()
{
	if (!bIsReloading)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	bIsReloading = false;
	// 취소된 장전은 탄약을 지급하지 않고, 재장전 완료 방송도 하지 않음.
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
	//bReloadWholeMagazine가 true라면 탄창 한번에 교환(돌격소총이나 레일건으로 예상)
	if (WeaponStats.bReloadWholeMagazine)
	{
		CurrentAmmo = ReloadTargetMagazine;
	}
	//bReloadWholeMagazine가 false라면 장전 한번에 총알 하나씩 장전됨(샷건)
	else
	{
		CurrentAmmo = FMath::Min(ReloadTargetMagazine, CurrentAmmo + 1);
	}

	OnAmmoChanged.Broadcast(CurrentAmmo, GetMagazineCapacity());

	OnReloadCompleted.Broadcast();

	if (!WeaponStats.bReloadWholeMagazine && CurrentAmmo < ReloadTargetMagazine && bExternalActionsAllowed)
	{
		StartReloaded();
	}
}

// ============================================================================
// 외부 강제 취소 / 상태 잠금
// ============================================================================

void ARGBaseWeapon::ForceCancelAllActions()
{
	StopFire();
	CancelReloaded();
	StopAiming();
}

void ARGBaseWeapon::SetExternalActionsAllowed(bool bAllowed)
{
	bExternalActionsAllowed = bAllowed;
	if (!bAllowed)
	{
		ForceCancelAllActions();
	}
}

// ============================================================================
// [P0] 공통 피해 (추후 약점 시스템을 위한 코드 추가 필요)
// ============================================================================

//강화 데미지 계산 
float ARGBaseWeapon::GetUpgradeDamageMultiplier() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks = Progression->GetUpgradeStackCount(FName(TEXT("DamageUp")));
			const float EffectAmount = Progression->GetUpgradeEffectAmount(FName(TEXT("DamageUp")));
			return 1.0f + (EffectAmount * Stacks);
		}
	}
	return 1.0f;
}
//거리에 따른 데미지 감쇠
float ARGBaseWeapon::CalculateDistanceFalloffMultiplier(float Distance) const
{
	if (WeaponStats.DamageFalloffStart <= 0.f && WeaponStats.DamageFalloffEnd <= 0.f)
	{
		return 1.f;
	}
	if (Distance <= WeaponStats.DamageFalloffStart)
	{
		return 1.f;
	}
	if (Distance >= WeaponStats.DamageFalloffEnd)
	{
		return WeaponStats.MinFalloffDamageMultiplier;
	}
	const float Range = FMath::Max(1.f, WeaponStats.DamageFalloffEnd - WeaponStats.DamageFalloffStart);
	const float Alpha = (Distance - WeaponStats.DamageFalloffStart) / Range;
	return FMath::Lerp(1.f, WeaponStats.MinFalloffDamageMultiplier, Alpha);
}

void ARGBaseWeapon::ApplyHitDamage(const FHitResult& Hit, float BaseDamage, const FVector& ShotStart, bool bIsDirectHit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyHitDamage] HitActor가 nullptr - 데미지 적용 안됨"));
		return;
	}

	const float UpgradeMult = GetUpgradeDamageMultiplier();
	const float Distance = FVector::Dist(ShotStart, Hit.ImpactPoint);
	const float FalloffMult = CalculateDistanceFalloffMultiplier(Distance);
	const bool bIsWeakSpot = Hit.Component.IsValid() && Hit.Component->ComponentHasTag(WeakSpotTag);
	const float WeakSpotMult = bIsWeakSpot ? WeakSpotDamageMultiplier : 1.0f;

	float FinalDamage = BaseDamage * UpgradeMult * FalloffMult * WeakSpotMult;

	UE_LOG(LogTemp, Warning,
		TEXT("[Weapon Damage] Target: %s | Base: %.1f | UpgradeMult: %.2f | Falloff: %.2f | WeakSpot: %.2f | Final: %.1f"),
		*HitActor->GetName(), BaseDamage, UpgradeMult, FalloffMult, WeakSpotMult, FinalDamage);

	TSubclassOf<UDamageType> DamageTypeClass = bIsDirectHit ? URGDirectHitDamageType::StaticClass() : UDamageType::StaticClass();

	UGameplayStatics::ApplyPointDamage(
		HitActor,
		FinalDamage,
		(Hit.ImpactPoint - ShotStart).GetSafeNormal(),
		Hit,
		OwningCharacter ? OwningCharacter->GetController() : nullptr,
		this,
		DamageTypeClass
	);

	OnWeaponHit.Broadcast(HitActor, FinalDamage, bIsWeakSpot, Hit.ImpactPoint);
}

float ARGBaseWeapon::GetFireInterval() const
{
	float FireInterval = WeaponStats.FireInterval;

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks =
				Progression->GetUpgradeStackCount(FName(TEXT("RateUp")));

			const float EffectAmount =
				Progression->GetUpgradeEffectAmount(FName(TEXT("RateUp")));

			// 발사 간격 감소
			FireInterval /= (1.0f + EffectAmount * Stacks);
		}
	}

	// 너무 작아지는 것 방지
	return FMath::Max(0.01f, FireInterval);
}

float ARGBaseWeapon::GetReloadTime() const
{
	float ReloadTime = WeaponStats.ReloadTime;

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks =
				Progression->GetUpgradeStackCount(FName(TEXT("RateUp")));

			const float EffectAmount =
				Progression->GetUpgradeEffectAmount(FName(TEXT("RateUp")));

			ReloadTime /= (1.0f + EffectAmount * Stacks);
		}
	}

	return FMath::Max(0.01f, ReloadTime);
}

int32 ARGBaseWeapon::GetMagazineCapacity() const
{
	float MagazineCapacity = WeaponStats.MagazineCapacity;

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks =
				Progression->GetUpgradeStackCount(FName(TEXT("MagazineUp")));

			const float EffectAmount =
				Progression->GetUpgradeEffectAmount(FName(TEXT("MagazineUp")));

			MagazineCapacity *= (1.0f + EffectAmount * Stacks);
		}
	}

	return FMath::Max(1, FMath::RoundToInt(MagazineCapacity));
}

void ARGBaseWeapon::HandleUpgradeApplied(FName UpgradeId, int32 NewStackCount)
{
	if (UpgradeId != FName(TEXT("MagazineUp")))
	{
		return;
	}

	const int32 OldCapacity = GetCapacityForStack(LastKnownMagazineStack);
	LastKnownMagazineStack = NewStackCount;
	const int32 NewCapacity = GetMagazineCapacity();

	const int32 CapacityDelta = NewCapacity - OldCapacity;
	if (CapacityDelta > 0)
	{
		CurrentAmmo = FMath::Clamp(CurrentAmmo + CapacityDelta, 0, NewCapacity);
		OnAmmoChanged.Broadcast(CurrentAmmo, NewCapacity);
	}
}

// 특정 스택 수 기준 탄창 계산 (GetMagazineCapacity 로직 재사용)
int32 ARGBaseWeapon::GetCapacityForStack(int32 Stacks) const
{
	float EffectAmount = 0.f;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			EffectAmount = Progression->GetUpgradeEffectAmount(FName(TEXT("MagazineUp")));
		}
	}
	float Capacity = WeaponStats.MagazineCapacity * (1.0f + EffectAmount * Stacks);
	return FMath::Max(1, FMath::RoundToInt(Capacity));
}