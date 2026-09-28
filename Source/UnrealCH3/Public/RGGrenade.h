// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGGrenade.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class USoundBase;
// 수류탄을 담당하는 클래스
// 투척 -> 이동 -> 신관 -> 폭발까지 처리

class UNiagaraSystem;

UCLASS()
class UNREALCH3_API ARGGrenade : public AActor
{
	GENERATED_BODY()

public:
	ARGGrenade();
	//함수에 static 을 쓰는 이유는 ThrowFromActor등은 수류탄이 존재하지 않는 시점에서 호출되는 함수이기 때문이다.
	// 존재하지 않는 인스턴스 내부의 함수를 불러 올 수 없기 때문에 ThrowFromActor를 외부 함수로 만들어줌 
	// 수류탄을 지금 던질 수 있는지 확인
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	static bool CanThrow(const UObject* WorldContext);

	// 수류탄 쿨타임이 얼마나 남았는지 반환
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	static float GetGrenadeCooldownRemaining(const UObject* WorldContext);

	// 수류탄을 생성하고 카메라 방향으로 던짐
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	static ARGGrenade* ThrowFromActor(AActor* Thrower,TSubclassOf<ARGGrenade> GrenadeClass,float ThrowSpeed = 1200.f);

	// 현재 날아가는 수류탄을 제거
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	static void DestroyActiveGrenade();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 신관 시간이 끝났을 때 호출
	UFUNCTION()
	void OnFuseTimerComplete();

	// 수류탄 폭발 처리
	void ExplodeGrenade();

	// 수류탄 충돌을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	USphereComponent* CollisionComponent;

	// 수류탄의 이동을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	UProjectileMovementComponent* ProjectileMovement;

	// 수류탄의 외형
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	UStaticMeshComponent* GrenadeMesh;

	// 폭발 데미지
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float ExplosionDamage = 60.f;

	// 폭발 범위
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float ExplosionRadius = 450.f;

	// 던진 후 폭발하기까지 걸리는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float FuseTime = 1.5f;

	// 수류탄 쿨타임
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float CooldownDuration = 8.f;

	// 신관 타이머
	FTimerHandle FuseTimerHandle;

	// 수류탄을 던진 캐릭터
	// 폭발 시 자기 자신에게 데미지가 들어가지 않도록 사용
	UPROPERTY()
	AActor* Thrower = nullptr;

private:

	// 마지막으로 수류탄을 던진 시간
	static float LastThrowTime;

	// 현재 존재하는 수류탄이 누군지 약한 참조
	// 전투가 끝날 떄 날아가는 수류탄을 지울 때 쓰는 코드
	// 엑터가 살아있으면 포인터 , 죽어있으면 nullptr 반환해서 댕글링포인터 막기
	static TWeakObjectPtr<ARGGrenade> ActiveGrenade;

	// 사운드
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Grenade|Sound")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Grenade|Effect")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;
};
