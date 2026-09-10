// Fill out your copyright notice in the Description page of Project Settings.

#include "RGGameModeBase.h"
#include "Engine/World.h"
#include "TimerManager.h" // 타이머 사용을 위해 추가

// ---------------------------------------------------------
// 생성자: 필수 세팅
// ---------------------------------------------------------
ARGGameModeBase::ARGGameModeBase()
{
    // Tick 함수 완전 비활성화 (성능 최적화)
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // 변수 초기화
    CurrentState = ERunState::Init;
    RemainingTime = 60.0f; // 기본 60초
    TargetKillsToClear = 10;
    CurrentKills = 0;
    bIsRunEnded = false;
    bIsStageCleared = false;
}

// ---------------------------------------------------------
// BeginPlay: 게임 시작 시 1회 호출
// ---------------------------------------------------------
void ARGGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    if (!VerifySystems())
    {
        UE_LOG(LogTemp, Error, TEXT("System validation failed! Cannot start the game."));
        ChangeRunState(ERunState::Pause);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("System validation complete. Starting combat."));
    ChangeRunState(ERunState::Combat);
}

// ---------------------------------------------------------
// 상태 전환 중앙 통제소 (타이머 일시정지/재개 포함)
// ---------------------------------------------------------
void ARGGameModeBase::ChangeRunState(ERunState NewState)
{
    if (CurrentState == ERunState::Result || CurrentState == ERunState::Loading)
    {
        UE_LOG(LogTemp, Warning, TEXT("Transition to ( %d ) ignored because current state is Result or Loading."), static_cast<int32>(NewState));
        return;
    }

    CurrentState = NewState;
    UE_LOG(LogTemp, Log, TEXT("Game state changed to: %d"), static_cast<int32>(CurrentState));

    // 상태에 따른 타이머 통제 (Tick 대체 로직)
    if (CurrentState == ERunState::Combat)
    {
        // 타이머가 설정된 적이 없으면 새로 세팅 (1초 간격 반복)
        if (!GetWorld()->GetTimerManager().TimerExists(RunTimerHandle))
        {
            GetWorld()->GetTimerManager().SetTimer(RunTimerHandle, this, &ARGGameModeBase::UpdateRunTimer, 1.0f, true);
        }
        // 설정은 되어있으나 일시정지 상태라면 재개
        else if (GetWorld()->GetTimerManager().IsTimerPaused(RunTimerHandle))
        {
            GetWorld()->GetTimerManager().UnPauseTimer(RunTimerHandle);
        }
    }
    else
    {
        // 전투 상태가 아니면(일시정지, 강화, 로딩 등) 타이머를 멈춤
        GetWorld()->GetTimerManager().PauseTimer(RunTimerHandle);
    }
}

// ---------------------------------------------------------
// 커스텀 타이머 함수 (1초마다 호출됨)
// ---------------------------------------------------------
void ARGGameModeBase::UpdateRunTimer()
{
    if (RemainingTime > 0.0f)
    {
        RemainingTime -= 1.0f;

        if (RemainingTime <= 0.0f)
        {
            RemainingTime = 0.0f;
            GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle); // 0초 도달 시 타이머 완전 정지
            CheckEndCondition(false, true); // 시간 초과 처리
        }
    }
}

// ---------------------------------------------------------
// 종료 조건 우선순위 판독기
// ---------------------------------------------------------
void ARGGameModeBase::CheckEndCondition(bool bIsPlayerDead, bool bIsTimeOut)
{
    if (bIsRunEnded) return;

    if (bIsPlayerDead)
    {
        bIsRunEnded = true;
        ExecuteGameOver(EDeathReason::Killed);
    }
    else if (CheckStageClearCondition())
    {
        bIsRunEnded = true;
        ExecuteStageClear();
    }
    else if (bIsTimeOut)
    {
        bIsRunEnded = true;
        ExecuteGameOver(EDeathReason::TimeOut);
    }
}

// ---------------------------------------------------------
// 스테이지 클리어 조건 검사
// ---------------------------------------------------------
bool ARGGameModeBase::CheckStageClearCondition()
{
    return (CurrentKills >= TargetKillsToClear);
}

// ---------------------------------------------------------
// 시스템 누락 검증 
// ---------------------------------------------------------
bool ARGGameModeBase::VerifySystems()
{
    return true;
}

// ---------------------------------------------------------
// 적 처치 시 호출 (이벤트 주도)
// ---------------------------------------------------------
void ARGGameModeBase::OnEnemyDied()
{
    if (bIsRunEnded) return;

    CurrentKills++;
    UE_LOG(LogTemp, Log, TEXT("Kill: %d / %d"), CurrentKills, TargetKillsToClear);

    if (CheckStageClearCondition())
    {
        CheckEndCondition(false, false);
    }
}

// ---------------------------------------------------------
// 결과 실행 로직
// ---------------------------------------------------------
void ARGGameModeBase::ExecuteGameOver(EDeathReason Reason)
{
    // 백그라운드에서 타이머가 계속 도는 것을 방지
    GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);

    ChangeRunState(ERunState::Result);

    if (Reason == EDeathReason::Killed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GAME OVER] Player died!"));
    }
    else if (Reason == EDeathReason::TimeOut)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GAME OVER] Time out!"));
    }
}

void ARGGameModeBase::ExecuteStageClear()
{
    GetWorld()->GetTimerManager().ClearTimer(RunTimerHandle);

    bIsStageCleared = true;
    ChangeRunState(ERunState::RestHub);
    UE_LOG(LogTemp, Warning, TEXT("[STAGE CLEAR] Conditions met! Spawning portal."));
}