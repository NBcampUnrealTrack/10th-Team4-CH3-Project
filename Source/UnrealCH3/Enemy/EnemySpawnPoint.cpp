// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemySpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Enemy/BaseEnemy.h"
#include "GameFramework/Character.h"


AEnemySpawnPoint::AEnemySpawnPoint()
{
	// 매 프레임 처리할 작업이 없으므로 Tick을 비활성화한다.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 포인트의 위치와 회전을 담당하는 루트 컴포넌트를 생성한다.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 에디터에서 적이 바라볼 방향을 확인하기 위한 화살표를 생성한다.
	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(SceneRoot);
	// 화살표를 녹색으로 표시한다.
	FacingArrow->SetArrowColor(FLinearColor::Green);
	// 에디터에서 방향을 쉽게 확인할 수 있도록 화살표 크기를 설정한다.
	FacingArrow->ArrowSize = 1.5f;
	// 실제 게임 화면에서는 화살표가 보이지 않도록 설정한다.
	FacingArrow->SetHiddenInGame(true);
	// 화살표가 다른 Actor와 충돌하지 않도록 설정한다.
	FacingArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 화살표가 NavMesh 생성 및 내비게이션 영역에 영향을 주지 않도록 설정한다.
	FacingArrow->SetCanEverAffectNavigation(false);

	// 스폰 포인트 Actor 자체가 다른 Actor의 이동이나 생성에 방해되지 않도록 충돌을 끈다.
	SetActorEnableCollision(false);
}

// 현재 포인트가 적 생성 위치로 사용 가능한지 확인한다.
bool AEnemySpawnPoint::CanBeUsedForSpawn() const
{
	// 포인트 전체 활성화와 스폰 용도 활성화가 모두 true여야 사용할 수 있다.
	return bEnabled && bUSeForSpawn;
}

// 현재 포인트가 적 복귀 위치로 사용 가능한지 확인한다.
bool AEnemySpawnPoint::CanBeUsedForRecovery() const
{
	// 포인트 전체 활성화와 복귀 용도 활성화가 모두 true여야 사용할 수 있다.
	return bEnabled && bUseForRecovery;
}

// 지정된 적 클래스의 캡슐 크기를 반영한 생성 Transform을 계산한다.
FTransform AEnemySpawnPoint::GetEnemySpawnTransform(TSubclassOf<ABaseEnemy> EnemyClass) const
{
	// 스폰 포인트의 월드 위치를 기본 생성 위치로 사용한다.
	FVector SpawnLocation = GetActorLocation();

	// 유효한 적 클래스가 전달되었는지 확인한다.
	if (EnemyClass)
	{
		// Actor를 실제로 생성하지 않고 클래스 기본 객체에서 캡슐 정보를 가져온다.
		const ABaseEnemy* EnemyCDO = EnemyClass->GetDefaultObject<ABaseEnemy>();

		// 적 클래스 기본 객체와 캡슐 컴포넌트가 모두 유효한지 확인한다.
		if (EnemyCDO && EnemyCDO->GetCapsuleComponent())
		{
			// 포인트를 발바닥 위치로 사용하기 위해 캡슐 반높이만큼 생성 위치를 올린다.
			SpawnLocation.Z += EnemyCDO->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		}
	}

	// 지면과 캡슐이 너무 밀착되는 것을 방지하기 위한 추가 높이를 적용한다.
	SpawnLocation.Z += GroundClearance;

	// 포인트의 Yaw만 사용하여 적의 생성 방향을 계산하고 Pitch와 Roll은 제거한다.
	const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

	// 계산된 회전, 위치, 기본 스케일을 사용하여 생성 Transform을 반환한다.
	return FTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
}

// 포인트와 플레이어 사이의 최소 거리 조건을 검사한다.
bool AEnemySpawnPoint::PassesSpawnDistanceCheck(const AActor* PlayerActor) const
{
	// 최소 거리 검사를 사용하지 않으면 무조건 통과시킨다.
	if (!bCheckMinimumPlayerDistance)
	{
		return true;
	}

	// 플레이어를 찾지 못한 경우 거리 검사를 수행할 수 없으므로 통과시킨다.
	if (!PlayerActor)
	{
		return true;
	}

	// 제곱근 계산을 피하기 위해 포인트와 플레이어 사이의 거리 제곱을 계산한다.
	const float DistanceSquared = FVector::DistSquared(GetActorLocation(), PlayerActor->GetActorLocation());

	// 실제 거리 대신 최소 거리의 제곱과 비교하여 조건 통과 여부를 반환한다.
	return DistanceSquared >= FMath::Square(MinimumPlayerDistance);
}
