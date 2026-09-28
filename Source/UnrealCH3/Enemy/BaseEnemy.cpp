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
// 낙사 복귀용
#include "Enemy/Interface/EnemyRecoveryProvider.h"
#include "AIController.h"
#include "TimerManager.h"
// Animation 추가용
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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

	//낙사 이동 방지설정
	GetCharacterMovement()->bCanWalkOffLedges = false;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = false;

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

void ABaseEnemy::FellOutOfWorld(const UDamageType& DamageType)
{
	if (RequestSafetyRecovery())
	{
		return;
	}

	Super::FellOutOfWorld(DamageType);
}

void ABaseEnemy::Attack()
{
	UE_LOG(LogTemp, Warning, TEXT("%s Attack"), *EnemyName.ToString());
	FName Socket(TEXT("Muzzle_01"));
	UNiagaraComponent* Particle = nullptr;
	if (AttackParticle && GetMesh()->DoesSocketExist(Socket))
	{
		const FTransform SocketTransform = GetMesh()->GetSocketTransform(Socket);
		Particle = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			AttackParticle,
			SocketTransform.GetLocation(),
			FRotator(0.0f, 90.0f, 0.0f),
			FVector(1.f),
			true
		);
	}

	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			AttackSound,
			GetActorLocation()
		);
	}


}

void ABaseEnemy::PlayAttackMontage()
{
	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}
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
	if (bIsDead || CurrentHP <= 0.f || DamageAmount <= 0.f)
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

	// 피해 받았을때 뭔가 할거 있을때 가상함수
	OnChangedHealth();

	// 피해 받은거 방송
	OnEnemyTakeDamage.Broadcast(CurrentHP);

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

void ABaseEnemy::OnChangedHealth()
{
	// TODO 피가 달았을때 뭔가 할거있으면
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

	OnEnemyDeath.Broadcast(this);

	if (URGProgressionSubsystem* Progression = GetGameInstance()->GetSubsystem<URGProgressionSubsystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 경험치 지급 요청: %.1f"), *GetName(), Exp);   // 추가
		Progression->GrantExperience(Exp);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] URGProgressionSubsystem을 찾을 수 없음!"), *GetName());   // 추가
	}

	// [추가] GameMode를 직접 참조하지 않고 정상 사망 사실만 외부에 알린다.
	// bIsDead 가드 덕분에 Die()당 한 번만 Broadcast된다.
	// [DEBUG] 정상 사망 Delegate가 실제로 Broadcast되는지 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("=== BASE ENEMY OnEnemyDeath BROADCAST : %s ==="),
		*GetName()
	);

	

	// [추가] 사망한 적은 즉시 화면/충돌에서 제거하고,
	// 같은 프레임의 데미지 피드백(히트마커/킬마커/데미지 숫자)이 끝난 뒤
	// 안전하게 실제 Actor가 파괴되도록 짧은 LifeSpan을 건다.
	// 여기서 Destroy()를 즉시 호출하지 않는 이유는 TakeDamage()가 Die() 이후에도
	// ReceiveDamageFeedback(..., this, ...)를 호출하기 때문이다.
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	if (AAIController* EnemyController = Cast<AAIController>(GetController()))
	{
		EnemyController->StopMovement();
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	SetLifeSpan(0.1f);

	UE_LOG(LogTemp, Log, TEXT("Character is Dead!"));
}

// ABaseEnemy.cpp
void ABaseEnemy::FaceTarget(float DeltaTime, float RotationSpeed)
{
	if (!TargetActor) return;

	FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero()) return;

	FRotator TargetRotation = ToTarget.Rotation();
	FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, RotationSpeed);
	SetActorRotation(NewRotation);
}

void ABaseEnemy::FaceLocation(FVector Location, float DeltaSeconds)
{
	FVector ToLocation = Location - GetActorLocation();
	ToLocation.Z = 0.0f;

	if (ToLocation.IsNearlyZero()) return;

	FRotator TargetRotation = ToLocation.Rotation();
	FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, 7.0f);
	SetActorRotation(NewRotation);
}

void ABaseEnemy::ShowAttackRangeLine()
{
	if (!AttackRangeMesh || !AttackRangeMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Is not Have AttackRangeMesh OR AttackRangeMesh"));
		return;
	}
	// 매번 새로 만들지 않도록 캐싱
	if (!WarningDynMat)
	{
		WarningDynMat = UMaterialInstanceDynamic::Create(WarningMaterialBase, this);
	}
	const float CubeSizeUU = AttackRangeMesh->GetStaticMesh()->GetBounds().BoxExtent.X * 2.0f; // 100uu
	const float ScaleXY = AttackMaxRange / CubeSizeUU;
	AttackRangeMesh->SetRelativeScale3D(FVector(ScaleXY, 0.5f, 0.1f));
	const float ForwardOffset = (CubeSizeUU * ScaleXY) * 0.5f; // = AttackRange
	AttackRangeMesh->SetRelativeLocation(FVector(ForwardOffset, 0.0f, 0.0f));
	AttackRangeMesh->SetMaterial(0, WarningDynMat);
	AttackRangeMesh->SetVisibility(true);

	// 시작 상태 초기화
	WarningDynMat->SetScalarParameterValue(TEXT("Opacity"), 0.0f);
}

void ABaseEnemy::UpdateAttackWarning(float Alpha)
{
	if (!WarningDynMat) return;

	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	// 시간이 지날수록 점점 진해지는 경고
	WarningDynMat->SetScalarParameterValue(TEXT("Opacity"), Alpha);

	// 노란색 -> 빨간색으로 색상 보간 (임박할수록 위험하게)
	FLinearColor WarnColor = FLinearColor::LerpUsingHSV(
		FLinearColor(1.0f, 0.8f, 0.0f), // 노랑
		FLinearColor(1.0f, 0.0f, 0.0f), // 빨강
		Alpha
	);
	WarningDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), WarnColor);
}

void ABaseEnemy::UpdateAttackWarningTransform()
{
	if (!AttackRangeMesh || !TargetActor) return;

	const FVector StartLocation = GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	FVector Direction = TargetLocation - StartLocation;
	Direction = Direction.GetSafeNormal(); // 길이 1인 방향 벡터로 정규화

	// 방향 벡터를 Yaw/Pitch/Roll 회전값으로 변환 (Z가 포함되어 있으므로 위아래 각도까지 반영됨)
	const FRotator LookAtRotation = Direction.Rotation();
	AttackRangeMesh->SetWorldRotation(LookAtRotation);
	const float CubeSizeUU = AttackRangeMesh->GetStaticMesh()->GetBounds().BoxExtent.X * 2.0f; // 100uu
	const float ScaleXY = AttackMaxRange / CubeSizeUU;
	// 회전된 방향을 따라 큐브를 앞으로 배치 (기존 로컬 오프셋 대신 월드 좌표로 계산)
	const float ForwardOffset = (CubeSizeUU * ScaleXY) * 0.5f;
	AttackRangeMesh->SetWorldLocation(StartLocation + Direction * ForwardOffset);
}

void ABaseEnemy::HideAttackRangeLine()
{
	if (AttackRangeMesh)
	{
		AttackRangeMesh->SetVisibility(false);
	}
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

void ABaseEnemy::SetAttackLocation(FVector Location)
{
	AttackLocation = Location;
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

float ABaseEnemy::GetHearingDistance()
{
	return HearingDistance;
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

FVector ABaseEnemy::GetAttackLocation()
{
	return AttackLocation;
}

void ABaseEnemy::SetState(EEnemyStateEnum State)
{
	CurrentState = State;
}

bool ABaseEnemy::CanAttack() const
{
	// 공격가능 && 죽지않음 && 복귀처리중 아님 && 복귀 후 공격 잠금 상태 아님
	return bCanAttack && !bIsDead && !bSafetyRecoveryInProgress && !bRecoveryAttackLocked;
}

bool ABaseEnemy::RequestSafetyRecovery()
{
	if (bIsDead || bSafetyRecoveryInProgress)
	{
		return false;
	}

	AActor* RecoveryProvider = GetOwner();

	if (!IsValid(RecoveryProvider))
	{
		return false;
	}

	if (!RecoveryProvider->GetClass()->ImplementsInterface(UEnemyRecoveryProvider::StaticClass()))
	{
		return false;
	}

	FTransform RecoveryTransform;

	const bool bFoundRecoveryTransform = IEnemyRecoveryProvider::Execute_FindRecoveryTransform(RecoveryProvider, this, RecoveryTransform);

	if (!bFoundRecoveryTransform)
	{
		return false;
	}

	BeginSafetyRecovery();

	const bool bTeleported = TeleportTo(RecoveryTransform.GetLocation(), RecoveryTransform.Rotator(), false, false);

	if (!bTeleported)
	{
		bSafetyRecoveryInProgress = false;
		bRecoveryAttackLocked = false;

		return false;
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	GetWorldTimerManager().ClearTimer(RecoveryAttackLockTimer);

	if (RecoveryAttackLockSeconds <= 0.0f)
	{
		FinishSafetyRecovery();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			RecoveryAttackLockTimer,
			this,
			&ABaseEnemy::FinishSafetyRecovery,
			RecoveryAttackLockSeconds,
			false
		);
	}

	return true;
}

bool ABaseEnemy::IsRecoveryAttackLocked() const
{
	return bRecoveryAttackLocked;
}

void ABaseEnemy::BeginSafetyRecovery()
{
	bSafetyRecoveryInProgress = true;
	bRecoveryAttackLocked = true;
	bCanAttack = false;

	TargetActor = nullptr;

	CurrentState = EEnemyStateEnum::Idle;

	if (AAIController* EnemyController = Cast<AAIController>(GetController()))
	{
		EnemyController->StopMovement();
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();

		GetCharacterMovement()->SetMovementMode(MOVE_None);
	}

	GetWorldTimerManager().ClearTimer(AttackCoolTimer);
	GetWorldTimerManager().ClearTimer(WarningTimer);

}

void ABaseEnemy::FinishSafetyRecovery()
{
	bSafetyRecoveryInProgress = false;
	bRecoveryAttackLocked = false;
	bCanAttack = true;

	if (GetCharacterMovement() && GetCharacterMovement()->MovementMode == MOVE_None)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}