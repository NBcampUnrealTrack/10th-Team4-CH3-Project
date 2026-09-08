// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGGameModeBase.generated.h"

// ---------------------------------------------------------
// 1. 상태(State) 및 열거형(Enum) 정의
// ---------------------------------------------------------
UENUM(BlueprintType)
enum class ERunState : uint8
{
    Init        UMETA(DisplayName = "초기화"),
    Combat      UMETA(DisplayName = "전투 중"),
    Pause       UMETA(DisplayName = "일시정지"),
    Upgrade     UMETA(DisplayName = "강화 중"),
    RestHub     UMETA(DisplayName = "휴식처(허브)"),
    Result      UMETA(DisplayName = "결과 화면"),
    Loading     UMETA(DisplayName = "로딩 중")
};

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
    Killed      UMETA(DisplayName = "사망"),
    TimeOut     UMETA(DisplayName = "시간 초과")
};

/**
 * 게임의 흐름(상태, 타이머, 승패 판정)을 관리하는 코어 게임모드
 */
UCLASS()
class UNREALCH3_API ARGGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARGGameModeBase();

protected:
    virtual void BeginPlay() override;

public:
    // ---------------------------------------------------------
    // 2. 상태 머신(State Machine) 관리
    // ---------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run State")
    ERunState CurrentState;

    UFUNCTION(BlueprintCallable, Category = "Run State")
    void ChangeRunState(ERunState NewState);

    // ---------------------------------------------------------
    // 3. 타이머 및 게임 종료 판정 (Tick 대체)
    // ---------------------------------------------------------
    UPROPERTY()
    FTimerHandle RunTimerHandle;

    UFUNCTION()
    void UpdateRunTimer(); // 1초마다 실행될 커스텀 타이머 함수

    UFUNCTION(BlueprintCallable, Category = "Run Logic")
    void CheckEndCondition(bool bIsPlayerDead, bool bIsTimeOut);

    // 스테이지 클리어 조건을 만족했는지 확인 (ex. 남은 적 0 등)
    UFUNCTION(BlueprintPure, Category = "Run Logic")
    bool CheckStageClearCondition();

    // ---------------------------------------------------------
    // 4. 시스템 검증 (Crash 방지)
    // ---------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "System")
    bool VerifySystems();

    // ---------------------------------------------------------
    // 5. 이벤트 주도 방식
    // ---------------------------------------------------------
    // 적이 죽었을 때 외부(AI 등)에서 호출해줄 함수
    UFUNCTION(BlueprintCallable, Category = "Run Logic")
    void OnEnemyDied();

protected:
    // 내부 실행 함수들
    void ExecuteGameOver(EDeathReason Reason);
    void ExecuteStageClear();

    // ---------------------------------------------------------
    // 6. 데이터 (변수)
    // ---------------------------------------------------------
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Run Data")
    float RemainingTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run Data")
    int32 CurrentKills;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run Data")
    int32 TargetKillsToClear; // 테스트용 목표 처치 수

    // 중복 이벤트 발생을 막기 위한 자물쇠 (Guard Clause) 변수
    bool bIsRunEnded;
    bool bIsStageCleared;
};