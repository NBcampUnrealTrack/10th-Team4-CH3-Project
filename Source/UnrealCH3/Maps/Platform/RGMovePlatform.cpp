// Fill out your copyright notice in the Description page of Project Settings.


#include "Maps/Platform/RGMovePlatform.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ARGMovePlatform::ARGMovePlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	SetRootComponent(PlatformMesh);

	PlatformMesh->SetMobility(EComponentMobility::Movable);
}

void ARGMovePlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector TopLocation = StartLocation + FVector(0.0f, 0.0f, MoveDistance);

	const FVector TargetLocation = bMovingUp ? TopLocation : StartLocation;

	const FVector NewLocation = FMath::VInterpConstantTo(
		GetActorLocation(),
		TargetLocation,
		DeltaTime,
		MoveSpeed
	);

	SetActorLocation(NewLocation);

	if (NewLocation.Equals(TargetLocation, 1.0f))
	{
		bMovingUp = !bMovingUp;
	}
}

// Called when the game starts or when spawned
void ARGMovePlatform::BeginPlay()
{
	Super::BeginPlay();
	
	StartLocation = GetActorLocation();
}



