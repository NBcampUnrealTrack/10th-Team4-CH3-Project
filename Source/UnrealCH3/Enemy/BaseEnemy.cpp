// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BaseEnemy.h"
#include "Enemy/AIEnemyController.h"
// 데미지 피드백 인터페이스 추가
#include "Combat/DamageFeedbackReceiver.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ABaseEnemy::ABaseEnemy()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AAIEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	ViewingAngle = 140;
	ViewingDistance = 3500.0f;

	CurrentState = EEnemyStateEnum::Idle;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 120.0f, 0.0f);
	bUseControllerRotationYaw = false;
}

// Called when the game starts or when spawned
void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	//설정된 최대 체력으로 시작
	CurrentHP = FMath::Max(0.f, MaxHP);

	if (MaxHP <= 0.f)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[%s] MaxHP must be greater than zero."),
			*GetName()
		);
	}

}

// Called to bind functionality to input
void ABaseEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float ABaseEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//죽은 적에게 중복 피해 방지
	if (bIsDead || CurrentHP<=0.f || DamageAmount <= 0.f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.f || bIsDead || CurrentHP <= 0.f)
	{
		return 0.f;
	}

	//피해 받기 전 체력 저장
	const float PreviousHP = CurrentHP;

	CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);

	//실제 피해량 계산
	const float AppliedDamage = PreviousHP - CurrentHP;

	UE_LOG(LogTemp, Log, TEXT("Enemy Dereased to : %f"), CurrentHP);

	// 현재 체력이 0 이하가 되면 죽음 처리
	const bool bKilledByThisDamage = CurrentHP <= 0.f;

	// 일반 피해는 적 위치, 포인트 피해는 실제 충돌 위치 사용
	FVector FeedbackLocation = GetActorLocation();

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointEvent =
			static_cast<const FPointDamageEvent&>(DamageEvent);

		FeedbackLocation = PointEvent.HitInfo.ImpactPoint;
	}

	if (bKilledByThisDamage)
	{
		Die();
	}

	//데미지를 준 객체에 결과 전달
	if (IsValid(DamageCauser))
	{
		if (IDamageFeedbackReceiver* Receiver =
			Cast<IDamageFeedbackReceiver>(DamageCauser))
		{
			Receiver->ReceiveDamageFeedback(
				AppliedDamage,
				bKilledByThisDamage,
				this,
				FeedbackLocation
			);
		}
	}

	return AppliedDamage;
}

void ABaseEnemy::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	//죽은 객체 상태 변화
	CurrentState = EEnemyStateEnum::Dead;

	UE_LOG(LogTemp, Log, TEXT("Character is Dead!"));
}

void ABaseEnemy::SetEnemyTurn(bool bIsTurn)
{
	GetCharacterMovement()->bOrientRotationToMovement = bIsTurn;
	bUseControllerRotationYaw = bIsTurn;
}

void ABaseEnemy::MoveAwayFromTarget(float DeltaSeconds)
{
	if (!TargetActor) return;
	FVector Direction = (GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
	Direction.Z = 0.0f;
	Direction = Direction.GetSafeNormal();

	AddMovementInput(Direction, 1.0f);
}

bool ABaseEnemy::IsTargetInAttackRange() const
{
	return false;
}

EEnemyStateEnum ABaseEnemy::GetEnemyState() const
{
	return CurrentState;
}

void ABaseEnemy::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

AActor* ABaseEnemy::GetTargetActor() const
{
	return TargetActor;
}

float ABaseEnemy::GetViewingAngle()
{
	return ViewingAngle;
}

float ABaseEnemy::GetViewingDistance()
{
	return ViewingDistance;
}

float ABaseEnemy::GetAttackMaxRange()
{
	return AttackMaxRange;
}

float ABaseEnemy::GetAttackMinRange()
{
	return AttackMinRange;
}

float ABaseEnemy::GetAttackDamage()
{
	return AttackDamage;
}

int ABaseEnemy::GetScore()
{
	return Score;
}

float ABaseEnemy::GetExp()
{
	return Exp;
}

void ABaseEnemy::SetState(EEnemyStateEnum State)
{
	CurrentState = State;
}