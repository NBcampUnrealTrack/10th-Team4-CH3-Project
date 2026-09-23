#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "KillZoneVolume.generated.h"

UCLASS()
class UNREALCH3_API AKillZoneVolume : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	AKillZoneVolume();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);
};
