// Fill out your copyright notice in the Description page of Project Settings.
#include "RGBaseWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

ARGBaseWeapon::ARGBaseWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
}

// Called when the game starts or when spawned
void ARGBaseWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// 데이터테이블에서 이 무기의 행을 찾아 스탯을 캐싱함
	// WeaponStatsTable / WeaponRowName은 각 무기 BP의 디테일 패널에서 따로 지정함.
	if (WeaponStatsTable && !WeaponRowName.IsNone())
	{
		static const FString ConTextString(TEXT("WeaponStatsLookUp"));
		if (const FWeaponStatsRow* Row = WeaponStatsTable->FindRow<FWeaponStatsRow>(WeaponRowName, ContextString))
		{
			WeaponStats = *Row;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s : WeaponStatsTable에서 RowName '%s' 를 찾지 못했습니다. 기본값을 사용합니다"), 
				*GetName(), *WeaponRowName.ToString());
		}
	}

	CurrentAmmo = WeaponStats.MagazineCapacity;
}

void ARGBaseWeapon::Fire()
{
}

bool ARGBaseWeapon::FireHitscan(const FVector& StartLocation, const FVector& FireDirection, float DamageOverride, TSet<AActor*>* AlreadyHitActors)
{
	return false;
}

void ARGBaseWeapon::ApplyHitDamage(const FHitResult& Hit, float BaseDamage, const FVector& ShotStart)
{
}

// Called every frame
void ARGBaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

