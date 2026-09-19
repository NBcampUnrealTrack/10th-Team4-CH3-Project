// Fill out your copyright notice in the Description page of Project Settings.
#include "RGShotgun.h"
#include "Enemy/BaseEnemy.h"
#include "Gamemode/RGProgressionSubsystem.h"

void ARGShotgun::StartFire()
{
	// 산탄총은 한 발씩 장전하는 방식이라, 장전 중 발사 버튼을 누르면
	// 진행 중이던 장전을 끝까지 기다리지 않고 즉시 취소한 뒤 바로 발사 가능한 상태로 전환.
	if (IsReloading())
	{
		CancelReloaded();
	}
	Super::StartFire();
}

void ARGShotgun::Fire()
{
	// 카메라 위치/방향은 돌격소총과 동일하게 한 번만 구함 
	FVector StartLocation, FireDirection;
	if (!GetMuzzleAimTransform(StartLocation, FireDirection))
	{
		return;
	}

	// PiercingPellet 보유 여부와 관통 데미지 배율을 펠릿 반복문 시작 전에 한 번만 조회
	bool bHasPiercingPellet = false;
	float PierceDamagePercent = 1.0f;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			bHasPiercingPellet = Progression->HasCoreUpgrade(FName(TEXT("PiercingPellet")));

			if (bHasPiercingPellet)
			{
				if (const FRGCoreUpgradeRow* Row = Progression->FindCoreUpgradeRow(FName(TEXT("PiercingPellet"))))
				{
					PierceDamagePercent = Row->DamagePercent;   // 관통 후 65%
				}
			}
		}
	}

	// 펠릿 하나마다 별도로 퍼짐(ApplySpread)을 적용해서 각기 다른 방향으로 히트스캔 발사.
	// 펠릿 하나당 피해량은 WeaponStats.BaseDamage(데이터테이블 값, 예: 8)를 그대로 사용 ->
	// 여러 발이 맞으면 ApplyHitDamage가 그만큼 여러 번 호출되어 결과적으로 총피해가 합산됨.
	for (int32 i = 0; i < PelletCount; ++i)
	{
		const FVector PelletDirection = ApplySpread(FireDirection);

		if (!bHasPiercingPellet)
		{
			// 강화 없으면 기존 그대로: 펠릿 하나가 한 명만 맞춤
			FHitResult Hit;
			const bool bHit = FireHitscan(StartLocation, PelletDirection, -1.f, nullptr, &Hit);
			if (bHit)
			{
				TryApplyKnockback(Hit.GetActor(), PelletDirection);   // 추가
			}
			continue;
		}

		// 강화 있으면: 이 펠릿 하나가 최대 2명(원본 + 관통 1회)까지 맞을 수 있음
		TSet<AActor*> AlreadyHitActors;
		FHitResult FirstHit;
		// 1번째 명중 - 원본 데미지
		const bool bFirstHit = FireHitscan(StartLocation, PelletDirection, -1.f, &AlreadyHitActors , &FirstHit);

		if (bFirstHit)
		{
			TryApplyKnockback(FirstHit.GetActor(), PelletDirection);   // 추가
			// 관통 1회만 - 65% 데미지. 반복문이 아니라 딱 한 번만 호출하므로 "추가 관통 없음" 자동 충족
			const float PierceDamage = WeaponStats.BaseDamage * PierceDamagePercent;
			FireHitscan(StartLocation, PelletDirection, PierceDamage, &AlreadyHitActors);
		}
	}
}

// FocusedSpread 보유 시 탄퍼짐 각도를 줄여줌 (분산 -45%)
FVector ARGShotgun::ApplySpread(const FVector& AimDirection) const
{
	float SpreadMultiplier = 1.0f;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
		{
			if (Progression->HasCoreUpgrade(FName(TEXT("FocusedSpread"))))
			{
				if (const FRGCoreUpgradeRow* Row = Progression->FindCoreUpgradeRow(FName(TEXT("FocusedSpread"))))
				{
					SpreadMultiplier = Row->DamagePercent;   // 0.55 = 분산 -45%. 필드는 재사용, 여기선 '탄퍼짐 배율'로 사용
				}
			}
		}
	}

	const float SpreadDegrees = (bIsAiming ? WeaponStats.AimSpread : WeaponStats.HipFireSpread) * SpreadMultiplier;

	if (SpreadDegrees <= 0.f)
	{
		return AimDirection;
	}

	const float SpreadRadians = FMath::DegreesToRadians(SpreadDegrees);
	return FMath::VRandCone(AimDirection, SpreadRadians);
}

// Knockback 강화: 펠릿에 맞은 적을 발사 방향으로 밀쳐냄
// RGShotgun.cpp - TryApplyKnockback()
void ARGShotgun::TryApplyKnockback(AActor* HitActor, const FVector& FireDirection) const
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor);
	if (!Enemy)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	URGProgressionSubsystem* Progression = GI ? GI->GetSubsystem<URGProgressionSubsystem>() : nullptr;
	if (!Progression || !Progression->HasCoreUpgrade(FName(TEXT("Knockback"))))
	{
		return;
	}

	const FRGCoreUpgradeRow* Row = Progression->FindCoreUpgradeRow(FName(TEXT("Knockback")));
	if (!Row)
	{
		return;
	}
	// 밀려나는 거리(cm)
	const float BaseKnockbackDistance = 150.f;   
	const float KnockbackDistance = BaseKnockbackDistance * Row->DamagePercent;

	FVector Direction = FireDirection;
	Direction.Z = 0.f;
	Direction.Normalize();

	const FVector NewLocation = Enemy->GetActorLocation() + Direction * KnockbackDistance;

	// sweep=true로 벽 등에 막히면 자연스럽게 멈춤
	Enemy->SetActorLocation(NewLocation, true);

	UE_LOG(LogTemp, Warning, TEXT("[Knockback] %s 밀쳐냄 (거리 %.0f)"), *Enemy->GetName(), KnockbackDistance);
}