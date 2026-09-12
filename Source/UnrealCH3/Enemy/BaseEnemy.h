// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enum/EnemyStateEnum.h"
#include "Enemy/Enum/EnemyAttackTypeEnum.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"


UCLASS()
class UNREALCH3_API ABaseEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    ABaseEnemy();
    virtual void BeginPlay() override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    //virtual void Attack();
    virtual void Die();
    bool IsTargetInAttackRange() const;
    /*bool IsTargetInDetectRange() const;
    void SetEnemyState(EEnemyState NewState);*/
    EEnemyStateEnum GetEnemyState() const;
    void SetTargetActor(AActor* NewTarget);
    AActor* GetTargetActor() const;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    float GetViewingAngle();
    float GetViewingDistance();
    float GetAttackMaxRange();
    float GetAttackDamage();
    int GetScore();
    float GetExp();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHP;            // 최대 체력

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentHP;        // 현재 체력

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackDamage;     // 공격력

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackMaxRange;   // 최대 공격 사거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackMinRange;   // 최소 공격 사거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Defense;          // 방어력

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Exp;              // 획득 경험치

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int Score;            // 처치 점수

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float ViewingAngle;     // 시야각

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float ViewingDistance;  // 시야 감지 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float HearingDistance;  // 청각 감지 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float TargetChangeTime; // 타겟 변경 대기 시간

    FTimerHandle TargetChangeTimer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    AActor* TargetActor;    // 현재 추적 중인 타겟(목표)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackCoolTime;   // 공격 쿨타임

    FTimerHandle AttackCoolTimer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float WarningTime;      // 공격 전 경고 시간

    FTimerHandle WarningTimer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    bool bIsDead = false;           // 사망 여부

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    EEnemyStateEnum CurrentState;   // 현재 상태
};