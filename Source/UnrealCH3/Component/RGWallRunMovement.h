#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGWallRunMovement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWallRunStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWallRunStopped);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALCH3_API URGWallRunMovement : public UActorComponent
{
	GENERATED_BODY()

public:	
	URGWallRunMovement();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	UFUNCTION()
	void OnCharacterLanded(const FHitResult& Hit);
	void CheckWallRun();
	void StartWallRun(const FHitResult& Hit, int32 Wall);
	void StopWallRun();
	void WallJump();
	void UpdateWallRun();
	void UpdateCameraTilt(float DeltaTime);

public:
	UPROPERTY(BlueprintAssignable)
	FOnWallRunStarted OnWallRunStarted;
	UPROPERTY(BlueprintAssignable)
	FOnWallRunStopped OnWallRunStopped;

private:
	UPROPERTY(VisibleAnywhere, Category = "Cache|Character")
	TObjectPtr<ACharacter> CharacterOwner;
	UPROPERTY(VisibleAnywhere, Category = "Cache|Component")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovementComponent;
	UPROPERTY(VisibleAnywhere, Category = "Cache|Component")
	TObjectPtr<class UCameraComponent> CharacterCameraComponent;

private:
	FTimerHandle WallRunTimerHandle;
	FVector WallNormal = FVector::ZeroVector;
	FVector LastWallNormal = FVector::ZeroVector;
	FVector LastWallRunLocation = FVector::ZeroVector;
	FVector StartWallRunDirection = FVector::ZeroVector;
	int32 CheckWall = 0;
	float DefaultGravity = 0.0f;
	float TargetRollRotation = 0.0f;
	float NextCanWallRunTime = 0.0f;
	float WallRunSpeed = 0.0f;
	float WallRunTime = 3.0f;
	float CanWallRunDistance = 0.0f;
	bool bIsWallRunning = false;
	
};
