// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enum/EnemyStateEnum.h"
#include "Enemy/Enum/EnemyAttackTypeEnum.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

class UBehaviorTree;

UCLASS()
class UNREALCH3_API ABaseEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    ABaseEnemy();
    UFUNCTION()
    virtual void BeginPlay() override;
    UFUNCTION()
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    UFUNCTION()
    virtual void Attack();
    UFUNCTION()
    virtual void WarningBeforAttack();
    UFUNCTION()
    virtual void CoolTime();
    UFUNCTION()
    virtual void Die();
    UFUNCTION()
    bool IsTargetInAttackRange() const;
    //bool IsTargetInDetectRange() const;
    
    UFUNCTION()
    EEnemyStateEnum GetEnemyState() const;
    UFUNCTION()
    void SetTargetActor(AActor* NewTarget);
    UFUNCTION()
    AActor* GetTargetActor() const;
    UFUNCTION()
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    UFUNCTION()
    void SetEnemyTurn(bool bIsTurn);
    UFUNCTION()
    void MoveAwayFromTarget(float DeltaSeconds);
    UFUNCTION()
    virtual void ShowAttackRangeLine();
    UFUNCTION()
    virtual void HideAttackRangeLine();
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FaceTarget(float DeltaTime, float RotationSpeed = 10.f);
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartAttackCooldown();
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ResetAttackCooldown();

    UBehaviorTree* GetEnemyBehaviorTree() const;
    float GetViewingAngle();
    float GetViewingDistance();
    float GetAttackMaxRange();
    float GetAttackMinRange();
    float GetAttackDamage();
    float GetWarningTime();
    int GetScore();
    float GetExp();

    //낙사 중 공격 방지를 위해 CanAttack 수정(조건 추가)
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool CanAttack() const;

    void SetState(EEnemyStateEnum State);
    void InitializeData();

    // 낙사 구역으로 떨어진 적 복귀
    virtual void FellOutOfWorld(const UDamageType& DamageType) override;

    // 낙사 시 복귀요청
    UFUNCTION(BlueprintCallable, Category = "Enemy|Recovery")
    bool RequestSafetyRecovery();

    // 복귀 직후 공격 금지 상태 판단
    UFUNCTION(BlueprintPure, Category = "Enemy|Recovery")
    bool IsRecoveryAttackLocked() const;

    //복귀 시 이동상태 초기화
    void BeginSafetyRecovery();

    // 복귀 보호 시간 이후 공격 제한 해제
    void FinishSafetyRecovery();


protected:
    UPROPERTY(EditDefaultsOnly, Category = "Enemy|Data")
    TObjectPtr<UDataTable> EnemyStateDataTable;
    UPROPERTY(EditDefaultsOnly, Category = "Enemy|Data")
    TObjectPtr<UDataTable> EnemyAttackTypeDataTable;
    UPROPERTY(EditDefaultsOnly, Category = "BehaivorTree")
    TObjectPtr<UBehaviorTree> EnemyBehaviorTree;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "AttackRangeMesh")
    UStaticMeshComponent* AttackRangeMesh;
    UPROPERTY()
    FName EnemyName;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    bool bCanAttack = true; //초기화 코드 추가했습니다

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    EEnemyStateEnum CurrentState;   // 현재 상태
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    EEnemyAttackType AttackType;    // 공격타입

    // 낙사 복귀 중복 처리 방지
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Recovery")
    bool bSafetyRecoveryInProgress = false;

    //복귀 직후 적이 플레이어를 바로 공격못하도록 제한
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Recovery")
    bool bRecoveryAttackLocked = false;

    // 복귀 후 공격제한 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Recovery", meta = (ClampMin = "0.0", Units = "s"))
    float RecoveryAttackLockSeconds = 1.0f;

    // 공격 제한 해제 타이머
    FTimerHandle RecoveryAttackLockTimer;
};