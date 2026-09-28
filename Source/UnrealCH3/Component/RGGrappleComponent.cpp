#include "Component/RGGrappleComponent.h"
#include "Gamemode/RGProgressionSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "CableComponent.h"

URGGrappleComponent::URGGrappleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}

void URGGrappleComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CharacterOwner = Cast<ACharacter>(GetOwner());
	if (CharacterOwner)
	{
		CharacterMovementComponent =CharacterOwner->GetCharacterMovement();
		CableComponent = CharacterOwner->FindComponentByClass<UCableComponent>();
	}
	if (!CableComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("CableComponent not found"));
	}

	if (CharacterMovementComponent)
	{
		DefaultGravity = CharacterMovementComponent->GravityScale;
	}
}

void URGGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsGrappling)
	{
		return;
	}

	if (bJustStarted)
	{
		bJustStarted = false;
	}
	else
	{
		float CurrentSpeed = CharacterMovementComponent->Velocity.Size();
		if (CurrentSpeed < 1000.0f)
		{
			StopGrapple();
			return;
		}
	}

	if (CableComponent)
	{
		FVector LocalTargetPos = CableComponent->GetComponentTransform().InverseTransformPosition(GrappleTargetLocation);
		CableComponent->EndLocation = LocalTargetPos;
		CableComponent->CableLength = LocalTargetPos.Size();
	}

	FVector CurrentLocation = CharacterOwner->GetActorLocation();
	if (FVector::DistSquared(GrappleTargetLocation, CurrentLocation) <= FMath::Square(StopDistance))
	{
		StopGrapple();
	}

	FVector GrappleDir = (GrappleTargetLocation - CurrentLocation).GetSafeNormal();
	FVector SwingDir = (GrappleDir + (FVector::UpVector * SwingMultiplier)).GetSafeNormal();
	CharacterMovementComponent->Velocity = SwingDir * GrappleSpeed;
}

float URGGrappleComponent::GetGrappleDelay() const
{
	float Delay = GrappleDelay;

	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (const URGProgressionSubsystem* Progression = GI->GetSubsystem<URGProgressionSubsystem>())
			{
				const int32 Stacks = Progression->GetUpgradeStackCount(FName(TEXT("GrabbingUp")));
				const float EffectAmount = Progression->GetUpgradeEffectAmount(FName(TEXT("GrabbingUp")));

				// 감소형 강화이므로 나눗셈 공식 사용 (FireInterval/ReloadTime과 동일 패턴)
				Delay /= (1.0f + EffectAmount * Stacks);
			}
		}
	}

	return FMath::Max(0.01f, Delay);
}

void URGGrappleComponent::StartGrapple()
{
	if (bIsGrappling || !bCanGrapple || !CheckTrace())
	{
		return;
	}

	bIsGrappling = true;
	bJustStarted = true;
	bCanGrapple = false;
	CharacterMovementComponent->GravityScale = 0.0f;
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
	OnGrappleStarted.Broadcast();

	if (CableComponent)
	{
		CableComponent->SetVisibility(true);
	}

	TWeakObjectPtr<URGGrappleComponent> WeakPtr = this;
	GetWorld()->GetTimerManager().SetTimer(GrappleTimerHandle, [WeakPtr]() { if (WeakPtr.IsValid()) { WeakPtr.Get()->bCanGrapple = true; } }, GetGrappleDelay(), false);
}

void URGGrappleComponent::StopGrapple()
{
	bIsGrappling = false;
	CharacterMovementComponent->GravityScale = DefaultGravity;
	FVector ForwardDir = CharacterOwner->GetActorForwardVector();
	ForwardDir.Z = 0.0f;
	ForwardDir.Normalize();

	if (CableComponent)
	{
		CableComponent->SetVisibility(false);
	}

	CharacterMovementComponent->Velocity = ForwardDir * (GrappleSpeed * 0.9f);
	OnGrappleStopped.Broadcast();
}

bool URGGrappleComponent::CheckTrace()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	CharacterOwner->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	FVector Start = ViewLocation;
	FVector ForwardDir = ViewRotation.Vector();
	FVector End = Start + (ForwardDir * MaxTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CharacterOwner);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
	if (bHit)
	{
		GrappleTargetLocation = HitResult.ImpactPoint;
		return true;
	}

	return false;
}


