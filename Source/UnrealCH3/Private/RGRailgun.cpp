// Fill out your copyright notice in the Description page of Project Settings.

#include "RGRailgun.h"
#include "Components/AudioComponent.h"
#include "gamemode/RGProgressionSubsystem.h"
#include "Engine/World.h"
#include "Enemy/BaseEnemy.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h" 

//충전 시작만 하는 함수
void ARGRailgun::StartFire() {

	// BeamFire 강화 보유 시 차징 없이 즉시 지속 발사 시작
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			if (Progression->HasCoreUpgrade(FName(TEXT("BeamFire"))))
			{
				StartBeamFire();
				return;
			}
		}
	}

	//차징중이거나 CanFire로직에 걸리면 발사 안됨
	if (bIsCharging) {
		return;
	}

	if (!CanFire()) {
		return;
	}
	// 발사 간격 시간 이후에만 발사 가능
	if (GetWorld()->GetTimeSeconds() < NextChargeAllowedTime) {
		return;
	}
	//충전중 ture
	bIsCharging = true;
	//차징 시작 시간을 계산하는게 주 임무
	ChargeStartTime = GetWorld()->GetTimeSeconds();
}
//충전 위한 변수 초기화 함수
void ARGRailgun::StopFire() {

	//BeamFire 강화 적용되어있으면 체크
	if (bIsBeamFiring)
	{
		StopBeamFire();
		return;
	}

	//충전 중 아님으로 설정
	bIsCharging = false;
	//차징 시작 시간 0초로 초기화
	ChargeStartTime = 0.f;
	Super::StopFire();
}
//충전이 얼마나 되었는지 0~1 사이 값을 반환
float ARGRailgun::GetChargeRatio01() const {
	if (!bIsCharging) {
		return 0.f;
	}
	// 실제 흐른 시간에 강화 배율을 곱해서 "체감 충전 시간"을 늘림
	const float RawElapsed = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	//차징 시간 ScaledElapsed에 저장
	const float ScaledElapsed = RawElapsed * GetChargeSpeedMultiplier();
	//차징 시간 0~MaxChargeTime으로 한정시킴
	const float ClampedElapsed = FMath::Clamp(ScaledElapsed, 0.f, MaxChargeTime);
	// 0~1비율로 반환
	return ClampedElapsed / MaxChargeTime;
}

float ARGRailgun::GetChargeFireRatio() const
{
	if (!FMath::IsFinite(MinChargeTime) || !FMath::IsFinite(MaxChargeTime) || MaxChargeTime <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(MinChargeTime / MaxChargeTime, 0.0f, 1.0f);
}

void ARGRailgun::ReleaseChargeAndFire() {
	if (!bIsCharging) {
		return;
	}
	//차징시간
	
	const float RawElapsed = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	const float Elapsed = RawElapsed * GetChargeSpeedMultiplier();
	//차징이 끝났으므로
	bIsCharging = false;
	//최소 충전 시간 미만족시 그냥 리턴
	if (Elapsed < MinChargeTime) {
		return;
	}
	//충전 시간 0~1 비율로 변환
	const float ClampedElapsed = FMath::Clamp(Elapsed, MinChargeTime, MaxChargeTime);
	const float ChargeRatio = (ClampedElapsed - MinChargeTime) / (MaxChargeTime - MinChargeTime);

	FireChargedShot(ChargeRatio);

	//탄환 한발 소모 및 브로드캐스팅
	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	OnAmmoChanged.Broadcast(CurrentAmmo, GetMagazineCapacity());
	//반동 트리거 방송
	OnShotFired.Broadcast();
	PlayFireSound();

	if (CurrentAmmo <= 0) {
		StartReloaded();
	}

	// 다음 충전은 발사 간격 이후에 가능.
	NextChargeAllowedTime = GetWorld()->GetTimeSeconds() + WeaponStats.FireInterval;
}
//실제로 총을 쏘는 부분
void ARGRailgun::FireChargedShot(float ChargeRatio01) {
	
	//탄퍼짐을 고려한 실제 발사 스프레드를 FireDirection에 넣음
	FVector StartLocation, FireDirection;
	if (!GetMuzzleAimTransform(StartLocation, FireDirection))
	{
		return;
	}
	FireDirection = ApplySpread(FireDirection);

	// 충전을 조금 했으면 MinDamage에 가깝게, 완전 충전했으면 MaxDamage에 가깝게
	const float Damage = FMath::Lerp(MinDamage, MaxDamage, ChargeRatio01);
	const float TraceRadius = GetFireTraceRadius();

	//같은 방향으로 여러번 쏘면서 이미 맞은 적은 무시 대상에 넣으면 관통 처리 완료.
	TSet<AActor*> AlreadyHitActors;
	for (int32 i = 0; i < MaxPierceCount; i++) {

		FHitResult Hit;
		const bool bHit = FireHitscan(StartLocation, FireDirection, Damage, &AlreadyHitActors , &Hit , true, TraceRadius);
		if (!bHit) {
			break;
		}
		TryTriggerHomingDamage(Hit, Damage);   // 추가: 이 관통 지점마다 유도 데미지 시도
	}
}

float ARGRailgun::GetChargeSpeedMultiplier() const
{
	float Multiplier = 1.0f;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			const int32 Stacks = Progression->GetUpgradeStackCount(FName(TEXT("RateUp")));
			const float EffectAmount = Progression->GetUpgradeEffectAmount(FName(TEXT("RateUp")));
			Multiplier += EffectAmount * Stacks; // 덧셈형이라 음수/0 될 일 없음
		}
	}
	return Multiplier;
}

void ARGRailgun::StartBeamFire()
{
	if (bIsBeamFiring || !CanFire())
	{
		return;
	}

	bIsBeamFiring = true;
	StartBeamSound();
	// BeamFireElapsedSinceLastAmmoConsumed는 여기서 리셋하지 않음
	// -> 끊어 쏴도 누적된 시간이 계속 유지되어 정확히 계산됨

	const float TickInterval = 0.1f;
	GetWorldTimerManager().SetTimer(BeamFireTimerHandle, this, &ARGRailgun::HandleBeamFireTick, TickInterval, true);
}

void ARGRailgun::StopBeamFire()
{
	bIsBeamFiring = false;
	GetWorldTimerManager().ClearTimer(BeamFireTimerHandle);
	StopBeamSound();
}

void ARGRailgun::HandleBeamFireTick()
{
	if (!bIsBeamFiring || !HasAmmo())
	{
		StopBeamFire();
		if (!HasAmmo())
		{
			StartReloaded();
		}
		return;
	}

	FVector StartLocation, FireDirection;
	if (!GetMuzzleAimTransform(StartLocation, FireDirection))
	{
		return;
	}

	const float TargetDPS = 80.f;
	const float TickInterval = 0.1f;
	const float DamageMult = GetUpgradeDamageMultiplier();
	const float TickDamage = TargetDPS * TickInterval * DamageMult;

	FHitResult Hit;
	const float TraceRadius = GetFireTraceRadius();
	const bool bHit = FireHitscan(StartLocation, FireDirection, TickDamage, nullptr, &Hit, true, TraceRadius);

	if (bHit)
	{
		TryTriggerHomingDamage(Hit, TickDamage);   // 추가
	}

	OnShotFired.Broadcast();

	// 시간 기반 탄약 소모: 누적 1초마다 탄약 1발 차감 (끊어 쏴도 정확히 누적됨)
	const float AmmoConsumeInterval = 1.0f;
	BeamFireElapsedSinceLastAmmoConsumed += TickInterval;

	if (BeamFireElapsedSinceLastAmmoConsumed >= AmmoConsumeInterval)
	{
		BeamFireElapsedSinceLastAmmoConsumed -= AmmoConsumeInterval;

		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
		OnAmmoChanged.Broadcast(CurrentAmmo, GetMagazineCapacity());

		if (CurrentAmmo <= 0)
		{
			StopBeamFire();
			StartReloaded();
		}
	}
}

void ARGRailgun::TryTriggerHomingDamage(const FHitResult& Hit, float DealtDamage)
{
	if (!Hit.GetActor())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	URGProgressionSubsystem* Progression = GI ? GI->GetSubsystem<URGProgressionSubsystem>() : nullptr;
	if (!Progression || !Progression->HasCoreUpgrade(FName(TEXT("HomingDamage"))))
	{
		return;
	}

	const FRGCoreUpgradeRow* Row = Progression->FindCoreUpgradeRow(FName(TEXT("HomingDamage")));
	if (!Row)
	{
		return;
	}

	// MaxTargets에 원래 맞은 대상까지 포함 -> 추가로 찾아야 할 인원은 (MaxTargets - 1)
	const int32 AdditionalTargetCount = Row->MaxTargets - 1;
	if (AdditionalTargetCount <= 0)
	{
		return;
	}

	AActor* OriginalTarget = Hit.GetActor();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OriginalTarget);
	if (OwningCharacter)
	{
		QueryParams.AddIgnoredActor(OwningCharacter);
	}

	const float HomingSearchRadius = 1000.f;   // 유도 탐색 범위
	GetWorld()->OverlapMultiByChannel(Overlaps, Hit.ImpactPoint, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(HomingSearchRadius), QueryParams);

	// 가까운 순으로 정렬
	Overlaps.Sort([&Hit](const FOverlapResult& A, const FOverlapResult& B)
		{
			const float DistA = A.GetActor() ? FVector::DistSquared(Hit.ImpactPoint, A.GetActor()->GetActorLocation()) : TNumericLimits<float>::Max();
			const float DistB = B.GetActor() ? FVector::DistSquared(Hit.ImpactPoint, B.GetActor()->GetActorLocation()) : TNumericLimits<float>::Max();
			return DistA < DistB;
		});

	int32 HitCount = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (HitCount >= AdditionalTargetCount)
		{
			break;
		}

		AActor* Candidate = Overlap.GetActor();
		if (!Cast<ABaseEnemy>(Candidate) || Candidate == OriginalTarget || Candidate == OwningCharacter)
		{
			continue;
		}

		// "동시에 맞는다" - 원본과 같은 데미지, 같은 순간에 적용
		UGameplayStatics::ApplyPointDamage(
			Candidate,
			DealtDamage,
			(Candidate->GetActorLocation() - Hit.ImpactPoint).GetSafeNormal(),
			Hit,
			OwningCharacter ? OwningCharacter->GetController() : nullptr,
			this,
			UDamageType::StaticClass()
		);

		HitCount++;
	}

	UE_LOG(LogTemp, Warning, TEXT("[HomingDamage] 추가 %d명 동시 타격 (원본 포함 총 %d명, 배율 100%%)"), HitCount, HitCount + 1);
}

float ARGRailgun::GetFireTraceRadius() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			if (Progression->HasCoreUpgrade(FName(TEXT("WideBeam"))))
			{
				if (const FRGCoreUpgradeRow* Row = Progression->FindCoreUpgradeRow(FName(TEXT("WideBeam"))))
				{	 // 기본 두께(cm)
					const float BaseRadius = 50.f;  
					// DT의 DamagePercent를 반경 배율로 사용
					return BaseRadius * Row->DamagePercent;  
				}
			}
		}
	}
	return 0.f;
}


void ARGRailgun::PlayFireSound()
{	// 빔 중에는 발사음 X 
	if (IsBeamFiring()) return;   
	// 일반 차지샷은 뗄 때 발사되므로 그때 1번 재생
	Super::PlayFireSound();       
}

void ARGRailgun::StartBeamSound()
{
	if (!BeamLoopSound) return;
	StopBeamSound();   // 혹시 남아있는 거 정리
	BeamAudio = UGameplayStatics::SpawnSoundAttached(BeamLoopSound, GetRootComponent());
}

void ARGRailgun::StopBeamSound()
{
	if (BeamAudio)

	{	 // 뚝 끊기지 않게
		BeamAudio->FadeOut(0.1f, 0.f);  
		BeamAudio = nullptr;
	}
}

void ARGRailgun::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 무기 교체/파괴 시 소리 남는 것 방지
	StopBeamSound();   
	Super::EndPlay(EndPlayReason);
}