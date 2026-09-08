// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle,
	Partrol,
	Chase,
	Attack,
	Hit,
	Dead
};

UENUM(BlueprintType)
enum class EEnemyAttackType : uint8
{
    melee,
    Ranged
};

UCLASS()
class UNREALCH3_API ABaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseEnemy();

    virtual void BeginPlay() override;
    // 함수는 사용할 것이지만 아직 생각을 못해서 주석처리 했습니다
    /*virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    virtual void Attack();
    virtual void Die();
    bool IsTargetInAttackRange() const;
    bool IsTargetInDetectRange() const;
    void SetEnemyState(EEnemyState NewState);
    EEnemyState GetEnemyState() const;
    void SetTargetActor(AActor* NewTarget);
    AActor* GetTargetActor() const*/;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FName name;                 // 적 id
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHP;                // 최대체력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentHP;            // 현재체력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackDamage;         // 공격력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackMaxRange;       // 공격 최대거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackMinRange;       // 공격 최소 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Defense;              // 방어력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Exp;                  // 경험치
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Score;                // 점수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float ViewingAngle;         // 시야각
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float ViewingDistance;      // 시야 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float HearingDistance;      // 청각 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float TargetChangeTime;     // 목표 유지 시간
    FTimerHandle TargetChangeTimer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    AActor* TargetActor;        // 목표 오브젝트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackCoolTime;       // 공격 쿨타임
    FTimerHandle AttackCoolTimer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float WarningTime;          // 공격 경고 시간
    FTimerHandle WarningTimeer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    bool bCanAttack;            // 공격 가능 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    bool bIsDead;               // 사망 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    EEnemyState CurrentState;   // 현재 상태
};
