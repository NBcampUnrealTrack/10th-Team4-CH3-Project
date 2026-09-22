#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGGrappleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleStopped);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALCH3_API URGGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URGGrappleComponent();

protected:
	virtual void BeginPlay() override;
	float GetGrappleDelay() const;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	FOnGrappleStarted OnGrappleStarted;
	FOnGrappleStopped OnGrappleStopped;

public:
	void StartGrapple();
	void StopGrapple();
	bool CheckTrace();
private:
	UPROPERTY(VisibleAnywhere, Category = "Cache|Character")
	TObjectPtr<ACharacter> CharacterOwner;
	UPROPERTY(VisibleAnywhere, Category = "Cache|Component")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovementComponent;

private:
	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleSpeed = 2200.0f;
	UPROPERTY(EditAnywhere, Category = "Grapple")
	float MaxTraceDistance = 2000.0f;
	UPROPERTY(EditAnywhere, Category = "Grapple")
	float StopDistance = 180.0f;
	UPROPERTY(EditAnywhere, Category = "Grapple")
	float SwingMultiplier = 0.25f;

	FTimerHandle GrappleTimerHandle;
	float GrappleDelay = 3.0f;
	FVector GrappleTargetLocation = FVector::ZeroVector;
	float DefaultGravity = 0.0f;
	bool bIsGrappling = false;
	bool bJustStarted = false;
	bool bCanGrapple = true;
};
