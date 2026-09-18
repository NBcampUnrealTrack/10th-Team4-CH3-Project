// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "RGBaseWeapon.h"
#include "RGShotgun.generated.h"

/*
 산탄총. 베이스 클래스와 다른 점은 딱 하나 -> 한 발(트리거 1회)에 여러 펠릿을 각각 퍼뜨려서 쏨.
 조준/재장전/피해 파이프라인은 전부 ARGBaseWeapon 걸 그대로 재사용.
 */
UCLASS()
class UNREALCH3_API ARGShotgun : public ARGBaseWeapon
{
	GENERATED_BODY()

protected:
	// 트리거 1회에 발사할 펠릿 개수. 산탄총 전용이라 여기 따로 둠.
	UPROPERTY(EditDefaultsOnly, Category = "Shotgun")
	int32 PelletCount = 8;

	virtual void Fire() override;
	//핵심 강화 2
	virtual FVector ApplySpread(const FVector& AimDirection) const override;  
	//적 넉백 함수 ( 핵심 강화 3)
	void TryApplyKnockback(AActor* HitActor, const FVector& FireDirection) const;


public:
	// 재장전 중 발사 버튼을 누르면, 진행 중이던 장전을 즉시 취소하고 바로 발사로 넘어감
	virtual void StartFire() override;
};