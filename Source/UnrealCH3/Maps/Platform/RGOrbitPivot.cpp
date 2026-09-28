// Fill out your copyright notice in the Description page of Project Settings.


#include "Maps/Platform/RGOrbitPivot.h"
#include "Components/SceneComponent.h"
#include "GameFramework/RotatingMovementComponent.h"

// Sets default values
ARGOrbitPivot::ARGOrbitPivot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	SetRootComponent(PivotRoot);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));

	RotatingMovement->RotationRate = FRotator(0.0f, 8.0f, 0.0f);


}


