// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BaseEnemy.h"
#include "Enemy/AIEnemyController.h"
#include "Enemy/DataTableStruct/StructEnemyState.h"
#include "Enemy/DataTableStruct/StructEnemyAttackType.h"
#include "UObject/ConstructorHelpers.h"
#include "BehaviorTree/BehaviorTree.h"
// 데미지 피드백 인터페이스 추가
#include "Combat/DamageFeedbackReceiver.h"
#include "Engine/DamageEvents.h"
#include "Gamemode/RGProgressionSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ABaseEnemy::ABaseEnemy()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AAIEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	static ConstructorHelpers::FObjectFinder<UDataTable> StateTable(TEXT("/Script/Engine.DataTable'/Game/Enemy/EnemyData.EnemyData'"));
	if (StateTable.Succeeded())
	{
		EnemyStateDataTable = StateTable.Object;
	}
	static ConstructorHelpers::FObjectFinder<UDataTable> AttackTypeTable(TEXT("/Script/Engine.DataTable'/Game/Enemy/EnemyAttackData.EnemyAttackData'"));
	if (AttackTypeTable.Succeeded())
	{
		EnemyAttackTypeDataTable = AttackTypeTable.Object;
	}

	CurrentState = EEnemyStateEnum::Idle;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 120.0f, 0.0f);
	bUseControllerRotationYaw = false;
}

// Called when the game starts or when spawned
void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	InitializeData();
	if (AAIEnemyController* EnemyCont = Cast<AAIEnemyController>(GetController()))
	{
		EnemyCont->UpdateSight();
	}
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

void ABaseEnemy::InitializeData()
{
	if (!EnemyStateDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Don't Find StateData Init"));
		return;
	}
	if (!EnemyAttackTypeDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Don't Find AttackData Init"));
		return;
	}

	if (FStructEnemyState* StateDataRow = EnemyStateDataTable->FindRow<FStructEnemyState>(EnemyName, TEXT("InitializeData")))
	{
		MaxHP = StateDataRow->MaxHP;
		Defense = StateDataRow->Defense;
		AttackDamage = StateDataRow->AttackDamage;
		Score = StateDataRow->Score;
		Exp = StateDataRow->EXP;
	}
	
	if (FStructEnemyAttackType* AttackDataRow = EnemyAttackTypeDataTable->FindRow<FStructEnemyAttackType>(EnemyName, TEXT("InitializeData")))
	{
		ViewingDistance = AttackDataRow->ViewingDistance;
		ViewingAngle = AttackDataRow->ViewingAngle;
		HearingDistance = AttackDataRow->HearingDistance;
		TargetChangeTime = AttackDataRow->TargetChangeTime;
		AttackMinRange = AttackDataRow->MinAttackRange;
		AttackMaxRange = AttackDataRow->MaxAttackRange;
		AttackCoolTime = AttackDataRow->CoolTime;
		WarningTime = AttackDataRow->WarningTime;
		AttackType = AttackDataRow->AttackType;
	}
}

void ABaseEnemy::Attack()
{
	UE_LOG(LogTemp, Warning, TEXT("%s Attack"), *EnemyName.ToString());
}

void ABaseEnemy::WarningBeforAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("%s Warning Attack"), *EnemyName.ToString());
}

void ABaseEnemy::CoolTime()
{
	UE_LOG(LogTemp, Warning, TEXT("%s CoolTime"), *EnemyName.ToString());
}


float ABaseEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Enemy TakeDamage 호출! Damage = %f, Causer = %s"),
		DamageAmount,
		DamageCauser ? *DamageCauser->GetName() : TEXT("None")
	);

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

	if (URGProgressionSubsystem* Progression = GetGameInstance()->GetSubsystem<URGProgressionSubsystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 경험치 지급 요청: %.1f"), *GetName(), Exp);   // 추가
		Progression->GrantExperience(Exp);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] URGProgressionSubsystem을 찾을 수 없음!"), *GetName());   // 추가
	}

	UE_LOG(LogTemp, Log, TEXT("Character is Dead!"));
}

// ABaseEnemy.cpp
void ABaseEnemy::FaceTarget(float DeltaTime, float RotationSpeed)
{
	if (!TargetActor) return;

	FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	//ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero()) return;

	FRotator TargetRotation = ToTarget.Rotation();
	FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, RotationSpeed);
	SetActorRotation(NewRotation);
}

void ABaseEnemy::ShowAttackRangeLine()
{
	FVector Center = GetActorLocation() + GetActorForwardVector() * (AttackMaxRange / 2.0f);
	FRotator Rot = GetActorRotation();
	AttackRangeMesh->SetWorldLocation(Center);
	AttackRangeMesh->SetWorldRotation(Rot);
	AttackRangeMesh->SetVisibility(true);
}

void ABaseEnemy::HideAttackRangeLine()
{
	AttackRangeMesh->SetVisibility(false);
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

void ABaseEnemy::StartAttackCooldown()
{
	bCanAttack = false;
	GetWorldTimerManager().SetTimer(
		AttackCoolTimer,
		this,
		&ABaseEnemy::ResetAttackCooldown,
		AttackCoolTime,
		false
	);
}

void ABaseEnemy::ResetAttackCooldown()
{
	bCanAttack = true;
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

UBehaviorTree* ABaseEnemy::GetEnemyBehaviorTree() const
{
	return EnemyBehaviorTree;
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

float ABaseEnemy::GetWarningTime()
{
	return WarningTime;
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

bool ABaseEnemy::CanAttack()
{
	return bCanAttack;
}
