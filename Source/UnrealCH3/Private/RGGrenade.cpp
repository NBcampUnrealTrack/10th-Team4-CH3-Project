// Fill out your copyright notice in the Description page of Project Settings.

#include "RGGrenade.h"

#include "Components/SphereComponent.h"
#include "Player/RGCharacter.h"
#include "RGBaseWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// 캐릭터에서 할 일 1. 캐릭터 헤더에서 TSubclassOf<ARGGrenade> GrenadeClass; 작성하고 언리얼 에디터에서 BP_Grenade 지정
// 2. 수류탄 투척 키 바인딩 3. 바인딩 함수 안에 ARGGrenade::ThrowFromActor(this, GrenadeClass); 호출


// 마지막으로 던진 시간을 아주 이전 시간으로 설정
// 게임 시작 후 바로 첫 번째 수류탄을 던질 수 있게 함
float ARGGrenade::LastThrowTime = -1000.f;

// 현재 날아가고 있는 수류탄을 저장
TWeakObjectPtr<ARGGrenade> ARGGrenade::ActiveGrenade;


// 수류탄의 기본 설정
ARGGrenade::ARGGrenade()
{
	PrimaryActorTick.bCanEverTick = false;

	// 충돌용 구체 생성
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	// 충돌 범위 설정
	CollisionComponent->InitSphereRadius(8.f);
	// Projectile 프로필 사용
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	// 충돌 컴포넌트를 루트로 설정
	RootComponent = CollisionComponent;


	// 수류탄의 외형 생성
	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	GrenadeMesh->SetupAttachment(RootComponent);
	// 메쉬 자체의 충돌은 사용하지 않음
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// UProjectileMovementComponent 의 기능 -> 매 프레임 Velocity 값을 보고 실제로 이동시켜주거나 포물선을 그리게 해준다.
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	// 어떤 컴포넌트를 움직일지 지정
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	// 속도
	ProjectileMovement->InitialSpeed = 1200.f;
	ProjectileMovement->MaxSpeed = 1200.f;
	// 이동 방향에 따라 수류탄 회전
	// false로 한다면 엑터의 회전값이 고정되게 됨
	ProjectileMovement->bRotationFollowsVelocity = true;
	// 벽에 부딪히면 튕기도록 설정
	ProjectileMovement->bShouldBounce = true;
	// 튕기는 정도
	ProjectileMovement->Bounciness = 0.4f;
	// 중력 적용
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}


// 수류탄이 게임에 생성됐을 때 호출
void ARGGrenade::BeginPlay()
{
	Super::BeginPlay();

	// 수류탄 중복 버그를 막기 위한 활성화 코드
	ActiveGrenade = this;

	// FuseTime이 지나면 폭발 함수 호출
	// FuseTime 1.5초로 세팅되어있음
	GetWorldTimerManager().SetTimer(
		FuseTimerHandle,
		this,
		&ARGGrenade::OnFuseTimerComplete,
		FuseTime,
		false
	);
}


// 수류탄이 제거될 때 호출
void ARGGrenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 현재 활성 수류탄이 자기 자신이라면 비워줌
	if (ActiveGrenade.Get() == this)
	{
		ActiveGrenade = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}


// 수류탄을 던질 수 있는지 확인
bool ARGGrenade::CanThrow(const UObject* WorldContext)
{
	// 남은 쿨타임이 0이면 던질 수 있음
	return GetGrenadeCooldownRemaining(WorldContext) <= 0.f;
}


// 수류탄 쿨타임을 계산
float ARGGrenade::GetGrenadeCooldownRemaining(const UObject* WorldContext)
{
	// 현재 월드를 가져옴
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;

	if (!World)
	{
		return 0.f;
	}

	if (World->GetTimeSeconds() < LastThrowTime)
	{
		LastThrowTime = -1000.0f;
	}

	// 마지막 투척 이후 얼마나 시간이 지났는지 계산
	float ElapsedTime = World->GetTimeSeconds() - LastThrowTime;

	// 쿨타임에서 지난 시간을 빼서 남은 시간을 계산
	// 음수가 되지 않도록 Max 사용
	return FMath::Max(0.f, 8.f - ElapsedTime);
}


// 캐릭터가 수류탄을 던지는 함수
ARGGrenade* ARGGrenade::ThrowFromActor(AActor* Thrower, TSubclassOf<ARGGrenade> GrenadeClass, float ThrowSpeed)
{
	// 던질 캐릭터나 수류탄 클래스가 없으면 종료
	// 쿨타임 중이어도 종료
	if (!Thrower || !GrenadeClass || !CanThrow(Thrower))
	{
		return nullptr;
	}


	// 현재 월드를 가져옴
	UWorld* World = Thrower->GetWorld();

	if (!World)
	{
		return nullptr;
	}


	// 투척 위치와 방향
	FVector StartLocation;
	FRotator ViewRotation;


	// 캐릭터의 컨트롤러를 가져옴
	APawn* Pawn = Cast<APawn>(Thrower);
	APlayerController* PlayerController =
		Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;


	// 플레이어 컨트롤러가 있다면 카메라 위치와 방향 사용
	if (PlayerController)
	{
		PlayerController->GetPlayerViewPoint(
			StartLocation,
			ViewRotation
		);
	}
	else
	{
		// 컨트롤러가 없으면 캐릭터의 위치와 회전 사용
		StartLocation = Thrower->GetActorLocation();
		ViewRotation = Thrower->GetActorRotation();
	}


	// 카메라가 바라보는 방향을 구함
	FVector ThrowDirection = ViewRotation.Vector();


	// 수류탄 생성 옵션
	FActorSpawnParameters SpawnParams;

	// 수류탄의 Owner를 던진 캐릭터로 설정
	SpawnParams.Owner = Thrower;

	// 충돌 때문에 생성되지 않는 상황을 방지
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


	// 수류탄 생성
	ARGGrenade* Grenade = World->SpawnActor<ARGGrenade>(GrenadeClass,StartLocation,ThrowDirection.Rotation(),SpawnParams);


	// 생성에 성공했다면
	if (Grenade)
	{
		// 누가 던졌는지 저장
		Grenade->Thrower = Thrower;


		// 수류탄을 카메라 방향으로 발사
		if (Grenade->ProjectileMovement)
		{
			Grenade->ProjectileMovement->Velocity = ThrowDirection.GetSafeNormal() * ThrowSpeed;
		}


		// 수류탄을 던진 순간부터 쿨타임 시작
		LastThrowTime = World->GetTimeSeconds();
	}


	// 생성된 수류탄 반환
	return Grenade;
}


// 현재 날아가고 있는 수류탄 제거
void ARGGrenade::DestroyActiveGrenade()
{
	// 현재 활성 수류탄이 존재하면
	if (ARGGrenade* Grenade = ActiveGrenade.Get())
	{
		// 수류탄 제거
		Grenade->Destroy();
	}
}


// 신관 시간이 끝났을 때 호출
void ARGGrenade::OnFuseTimerComplete()
{
	ExplodeGrenade();
}


// 수류탄 폭발 처리
void ARGGrenade::ExplodeGrenade()
{
	TArray<AActor*> IgnoreActors;
	if (Thrower)
	{
		IgnoreActors.Add(Thrower);
	}
	IgnoreActors.Add(this);   // 추가: 수류탄 자신이 판정에 끼지 않게

	AController* InstigatorController = nullptr;
	if (APawn* Pawn = Cast<APawn>(Thrower))
	{
		InstigatorController = Pawn->GetController();
	}

	// 추가: 피드백(히트마커/데미지 숫자)을 받을 대상 = 플레이어의 현재 무기
	AActor* FeedbackCauser = this;
	if (ARGCharacter* Character = Cast<ARGCharacter>(Thrower))
	{
		if (ARGBaseWeapon* Weapon = Character->GetCurrentWeapon())
		{
			FeedbackCauser = Weapon;
		}
	}

	UGameplayStatics::ApplyRadialDamageWithFalloff(
		this,
		ExplosionDamage,
		ExplosionDamage,
		GetActorLocation(),
		ExplosionRadius,
		ExplosionRadius,
		1.f,
		UDamageType::StaticClass(),
		IgnoreActors,
		FeedbackCauser,        // ← this 대신 무기
		InstigatorController
	);

	Destroy();
}