#include "Gamemode/KillZoneVolume.h"
#include "Player/RGCharacter.h"

AKillZoneVolume::AKillZoneVolume()
{
}

void AKillZoneVolume::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AKillZoneVolume::OnOverlapBegin);
}

void AKillZoneVolume::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
	{
		if (ARGCharacter* Player = Cast<ARGCharacter>(OtherActor))
		{
			Player->ReturnCheckPoint();
		}
	}
}
