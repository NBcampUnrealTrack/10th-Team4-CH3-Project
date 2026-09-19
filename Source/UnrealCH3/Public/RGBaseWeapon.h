#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGWeaponStats.h"
#include "Combat/DamageFeedbackReceiver.h"
#include "RGBaseWeapon.generated.h"

class USkeletalMeshComponent;
class UDataTable;
class ACharacter;

//UI 바인딩용 델리게이트 _ 리로드시작 리로드끝 에이밍 총알갯수변경 시 델리게이트 호출 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, MagazineCapacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimingChanged, bool, bNowAiming);

//UI담당자 추가 델리게이트 - 재장전캔슬 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadCanceled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponShotFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponShotFiredStop);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponDamageConfirmed, float, AppliedDamage, bool, bKilled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponDamageNumberRequested, float, AppliedDamage, AActor*, TargetActor, FVector, WorldLocation);

//명중 시 맞은 대상 , 최종 피해량 , 약점 bool , 맞은 위치 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnWeaponHit, AActor*, HitActor, float, FinalDamage, bool, bIsWeakspot, FVector, HitLocation);

//직접 공격 용 데이터 타입
//데이터 코어에는 강화로인한 도탄 폭발 스플래쉬 데미지는 받지 않으므로 
UCLASS()
class UNREALCH3_API URGDirectHitDamageType : public UDamageType
{
	GENERATED_BODY()
};


UCLASS()
// 데미지 피드백 인터페이스 상속 추가
class UNREALCH3_API ARGBaseWeapon : public AActor, public IDamageFeedbackReceiver
{
	GENERATED_BODY()

public:
	ARGBaseWeapon();
	virtual void Tick(float DeltaTime) override;

	// ===== UI가 바인딩할 델리게이트 (BlueprintAssignable = BP에서도 Bind Event 가능) =====
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadStarted OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadCompleted OnReloadCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAimingChanged OnAimingChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadCanceled OnReloadCanceled;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponShotFired OnShotFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponShotFiredStop OnShotFiredStop;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponHit OnWeaponHit;
	//=======================================================================================

	//명중, 킬마커 정보 전달
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponDamageConfirmed OnDamageConfirmed;

	//디버그 발사 충돌 검사용 라인트레이스
	UPROPERTY(EditAnywhere, Category = "Weapon|Debug")
	bool bShowWeaponTraceDebug = false;

	//데미지 숫자 정보 전달
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponDamageNumberRequested OnDamageNumberRequested;

	// 데미지 피드백 전달
	virtual void ReceiveDamageFeedback(float AppliedDamage, bool bKilled, AActor* TargetActor, const FVector& WorldLocation) override;

protected:

	virtual void BeginPlay() override;

	// ===== 총알 발사 관련 함수 =====
	virtual void Fire();
	//라인트레이스 -> 맞았으면 ApplyHitDamage호출 ( AlreadyHitActors 를 넘기면 이 안의 엑터는 다시 처리하지 않음 ( 관통 무기용 중복 방지) )
	//맞췄으면 true 반환 못맞췄으면 false 반환
	virtual bool FireHitscan(const FVector& StartLocation, const FVector& FireDirection, float DamageOverride, TSet<AActor*>* AlreadyHitActors , FHitResult* OutHit = nullptr , bool bTriggerCoreEffects = true);
	//피해 로직 : 기본 피해 * 강화 배율 * 거리 감쇠(선택) -> 최종 피해 전달.
	// if 강화 X 에 거리감쇠 효과 0으로 한다면 -> 기본피해 == 최종피해
	// 실제 체력 차감은 맞은 대상 (적) 에서 처리하는 것으로 구현 + 적 체력 여기서 건드리지 않음
	virtual void ApplyHitDamage(const FHitResult& Hit, float BaseDamage, const FVector& ShotStart, bool bIsDirectHit , bool bTriggerCoreEffects = true);
	// 발사 시작 지점/방향을 구함 기본은 캐릭터의 카메라 기준. 실패 시 false.
	virtual bool GetMuzzleAimTransform(FVector& OutStart, FVector& OutDirection) const;
	//재장전 타이머 끝났을 때 실제 재장전 처리
	virtual void CompleteReload();
	//강화 배율 계산
	virtual float GetUpgradeDamageMultiplier() const;
	//거리 감쇠 배율 계산
	virtual float CalculateDistanceFalloffMultiplier(float Distance) const;
	// 조준 여부(bIsAiming)에 따라 탄퍼짐 효과 적용
	virtual FVector ApplySpread(const FVector& AimDirection) const;
	// ===========================

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	FName WeakSpotTag = FName(TEXT("Weakspot"));

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	float WeakSpotDamageMultiplier = 1.5f;

	//루트 컴포넌트 생성
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;

	//데이터테이블 연동
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	UDataTable* WeaponStatsTable;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	FName WeaponRowName;

	//===== 위 데이터 테이블에서 읽고 스탯 연동 =====
	//TGWeaponStats.h 에 정의된 FWeaponStatsRow 구조체 인스턴스 생성
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FWeaponStatsRow WeaponStats;
	//트레이스 감지 범위
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Trace")
	float TraceRange = 10000.f;
	//라인트레이스 시스템이 감지하는 채널을 Visiblility 채널로 설정함.
	//드롭다운으로 채널 바꾸기 가능
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	int32 CurrentAmmo = 0;
	//에이밍
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsAiming = false;
	//리로딩
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsReloading = false;
	// 발사 버튼이 눌려 있는지 (연타 중복 발사 방지 + 발사 루프 유지)
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bWantsToFire = false;
	// 런 시작/입력 가능/사망/로딩/강화 선택 중/결과 화면 시 
	// 게임모드·캐릭터 등에서 가져올 것 (외부 액션이 허용된 상태)
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bExternalActionsAllowed = true;

	ACharacter* OwningCharacter = nullptr;
	//타이머 생성용
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;

	UFUNCTION()
	void HandleUpgradeApplied(FName UpgradeId, int32 NewStackCount);
	UPROPERTY()
	int32 LastKnownMagazineStack = 0;
private:
	// 타이머 관련 함수들
	UFUNCTION()
	void OnReloadTimerComplete();

	void HandleFireTick();
	void StartFireTimer();
	void StopFireTimer();
	bool HasAmmo() const { return CurrentAmmo > 0; }

public:
	// ===== 캐릭터/입력에서 호출하는 함수들 =====
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartAiming();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopAiming();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartReloaded();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void CancelReloaded();

	// 밀치기/낙사/수류탄/강화 선택/전투 종료 등에서 공통으로 호출하는 강제 취소
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void ForceCancelAllActions();

	/*
	  게임모드/캐릭터가 "지금 무기 행동이 전부 금지되어야 하는 상태"(사망, 로딩, 강화 선택, 결과 화면 등)를
	  진입/해제할 때 호출합니다. false가 되는 순간 발사/조준/재장전을 전부 강제 취소합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetExternalActionsAllowed(bool bAllowed);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetOwningCharacter(ACharacter* NewOwner);

	// ===== 상태 조회 =====
	//(델리게이트 시스템이 있으면 재장전 , 에임 , 총알변경 등은 여기서 안가져와도됨)
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanReloaded() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAiming() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReloading() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetADSFOV() const { return WeaponStats.ADSFOV; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadProgress() const;

	float GetFireInterval() const;
	float GetReloadTime() const;
	int32 GetMagazineCapacity() const;

	//버그 수정 시 추가로 들어감
	UPROPERTY()
	int32 ReloadTargetMagazine = 0;
	int32 GetCapacityForStack(int32 Stacks) const;
	UPROPERTY()
	float LastFireTime = -1000.f;

	//=============== 핵심 강화 함수 ===================
	protected:
		// 핵심 강화 효과 분기 진입점
		// bTriggerCoreEffects: 후속 효과(도탄/전이 등)가 자기 자신을 또 트리거하지 않도록 막는 플래그
		void TryTriggerCoreUpgradeEffects(const FHitResult& Hit, float DealtDamage, const FVector& ShotStart);
		//개별 효과 구현
		void TriggerDoubleShot(const FHitResult& Hit, float DealtDamage, const FVector& ShotStart);
		void TriggerChainPulse(const FHitResult& Hit, float DealtDamage, const FVector& ShotStart);
		void TriggerExplosiveRound(const FHitResult& Hit, float DealtDamage, const FVector& ShotStart);
};