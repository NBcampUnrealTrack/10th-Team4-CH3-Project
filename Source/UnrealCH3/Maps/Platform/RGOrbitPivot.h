// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGOrbitPivot.generated.h"

class USceneComponent;
class URotatingMovementComponent;

UCLASS()
class UNREALCH3_API ARGOrbitPivot : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGOrbitPivot();

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit")
	TObjectPtr<USceneComponent> PivotRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

};
