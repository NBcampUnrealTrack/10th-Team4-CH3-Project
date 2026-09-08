#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGWeaponStats.h"
#include "RGBaseWeapon.generated.h"

class USkeletalMeshComponent;
class UDataTable;
class ACharacter;

UCLASS()
class UNREALCH3_API ARGBaseWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	ARGBaseWeapon();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	//총알 발사 로직
	virtual void Fire();
	//라인트레이스 -> 맞았으면 ApplyHitDamage호출 ( AlreadyHitActors 를 넘기면 이 안의 엑터는 다시 처리하지 않음 ( 관통 무기용 중복 방지) )
	//맞췄으면 true 반환 못맞췄으면 false 반환
	virtual bool FireHitscan(const FVector& StartLocation, const FVector& FireDirection, float DamageOverride, TSet<AActor*>* AlreadyHitActors);
	//피해 로직 : 기본 피해 * 강화 배율 * 거리 감쇠(선택) -> 최종 피해 전달.
	// if 강화 X 에 거리감쇠 효과 0으로 한다면 -> 기본피해 == 최종피해
	// 실제 체력 차감은 맞은 대상 (적) 에서 처리하는 것으로 구현 + 적 체력 여기서 건드리지 않음
	virtual void ApplyHitDamage(const FHitResult& Hit, float BaseDamage, const FVector& ShotStart);
	// 발사 시작 지점/방향을 구함 기본은 캐릭터의 카메라 기준. 실패 시 false.
	virtual bool GetMuzzleAimTransform(FVector& OutStart, FVector& OutDirection) const;
	//재장전 타이머 끝났을 때 실제 재장전 처리
	virtual void CompleteReload();
	//루트 컴포넌트 생성
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;

	//데이터테이블 연동
	UPROPERTY(EditDefaltsOnly , Category = "Weapon|Stats")
	UDataTable* WeaponStatsTable;
	UPROPERTY(EditDefaltsOnly, Category = "Weapon|Stats")
	FName WeaponRowName;

	//위 데이터 테이블에서 읽고 스탯 연동
	//TGWeaponStats.h 에 정의된 FWeaponStatsRow 구조체 인스턴스 생성
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FWeaponStatsRow WeaponStats;
	//트레이스 감지 범위
	UPROPERTY(BlueprintReadOnly , Category = "Weapon|Trace")
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

private:
	UFUNCTION()
	void OnReloadTImerComplete();

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
};
