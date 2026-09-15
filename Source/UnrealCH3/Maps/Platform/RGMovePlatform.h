// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGMovePlatform.generated.h"

class UStaticMeshComponent;

UCLASS()
class UNREALCH3_API ARGMovePlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGMovePlatform();

protected:
	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Movement", meta = (ClampMin = "0.0"))
	float MoveDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeed = 100.0f;

	FVector StartLocation;

	bool bMovingUp = true;

};
