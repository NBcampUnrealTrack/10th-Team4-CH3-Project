// Fill out your copyright notice in the Description page of Project Settings.
#include "RGProgressionSubsystem.h"
#include "RGEXPLevelRow.h"
#include "Engine/DataTable.h"
#include "Player/RGCharacter.h"
#include "RGBaseWeapon.h"
#include "Kismet/GameplayStatics.h"


// =============================경험치 및 일반 강화 ====================
//GameMode의 BeginPlay에서 초기화
//이미 만들어진 경험치커브 데이터테이블을 세팅해준다.
void URGProgressionSubsystem::InitializeExperienceCurve(UDataTable* InExperienceCurveTable)
{
	ExperienceCurveTable = InExperienceCurveTable;
}
//업그레이드 데이터 테이블 세팅
void URGProgressionSubsystem::InitializeGeneralUpgrades(UDataTable* InGeneralUpgradeTable)
{
	GeneralUpgradeTable = InGeneralUpgradeTable;
}

void URGProgressionSubsystem::InitializeCoreUpgrades(UDataTable* InCoreUpgradeTable) {
	CoreUpgradeTable = InCoreUpgradeTable;
}

//현재 레벨이 몇 레벨인지 데이터테이블에서 찾아오는 함수.
//레벨별 데이터테이블 행을 반환한다.
const FRGEXPLevelRow* URGProgressionSubsystem::FindCurrentLevelRow() const
{
	if (!ExperienceCurveTable)
	{
		return nullptr;
	}
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	return ExperienceCurveTable->FindRow<FRGEXPLevelRow>(RowName, TEXT("ExperienceCurveLookup"));
}

//적 AI에서 호출하는 함수. Amount에 EXP값을 넣어서 호출.
//경험치가 없다면 0 넣어서 호출하거나 미호출.
void URGProgressionSubsystem::GrantExperience(float Amount)
{
	// 0 이하로 넘어오면 그냥 무시 (보스처럼 "경험치 없음"인 적은 애초에 이 함수를
	// 호출하지 않거나 0을 넘기면 되므로, 별도의 "보스인지 아닌지" 분기가 필요 없음)
	if (Amount <= 0.f)
	{
		return;
	}

	CurrentExperience += Amount;

	// 한 번에 여러 레벨을 올릴 수도 있으므로(예: 갑자기 큰 경험치를 얻은 경우) 반복 처리.
	// "연속 레벨업 대기열"이 바로 이 while문 + PendingLevelUpCount임.
	//TryLevelUpOnce() (bool반환 함수) 자체를 반복하는 반복문
	while (TryLevelUpOnce())
	{
	}

	const FRGEXPLevelRow* Row = FindCurrentLevelRow();
	const float RequiredExperience = Row ? Row->RequiredExperience : 0.f;
	//경험치를 획득했을 때 방송 -> 필요 경험치와 현재 경험치를 UI에서 불러와서 경험치 바 갱신
	OnExperienceChanged.Broadcast(CurrentExperience, RequiredExperience, CurrentLevel);

	// 레벨업이 하나라도 발생했는데 아직 카드 선택 화면이 안 떠 있다면 지금 하나 띄움
	PresentNextPendingLevelUpIfAny();
}

//디버깅용 강제 레벨업
void URGProgressionSubsystem::DebugForceLevelUp()
{
	UE_LOG(LogTemp, Warning, TEXT("DebugForceLevelUp 호출됨"));
	CurrentLevel += 1;
	PendingLevelUpCount += 1;
	PresentNextPendingLevelUpIfAny();
}

bool URGProgressionSubsystem::TryLevelUpOnce()

{
	const FRGEXPLevelRow* Row = FindCurrentLevelRow();
	// 테이블에 더 이상 다음 레벨 정보가 없음 (예: 6레벨 이상) -> 여기서 진행 멈춤
	if (!Row)
	{
		
		return false;
	}
	// 아직 다음 레벨까지 부족함
	if (CurrentExperience < Row->RequiredExperience)
	{
		return false; 
	}

	// 초과 경험치는 버리지 않고 다음 레벨로 그대로 이월
	CurrentExperience -= Row->RequiredExperience;
	CurrentLevel += 1;
	PendingLevelUpCount += 1;

	return true; // 레벨업 성공 -> 혹시 한 번 더 오를 수 있는지 바깥의 while문이 다시 확인함
}

//정해진 Count만큼 UpgradeOption 를 구조체의 플랫에 맞게 세팅하고 세팅된 구조체 배열 반환
//구조체는 UI의 카드와 매치되게 만들어져있음
TArray<FRGUpgradeOption> URGProgressionSubsystem::GenerateUpgradeOptions(int32 Count) const
{
	TArray<FRGUpgradeOption> Result;

	if (!GeneralUpgradeTable)
	{
		return Result;
	}
	// 현재 무기를 한 번만 가져와서 전체 후보 순회에 재사용
	ARGBaseWeapon* EquippedWeapon = GetCurrentEquippedWeapon();

	TArray<FName> ValidRowNames;
	static const FString ContextString(TEXT("UpgradeCandidateGen"));
	//강화 테이블 가져와서 행 이름 가져옴
	for (const FName& RowName : GeneralUpgradeTable->GetRowNames())
	{
		//이름이 일치하는 행 불러와서 예외사항이 없으면 ValidRowNames에 해당하는 행 이름 넣음
		const FRGGeneralUpgradeRow* Row = GeneralUpgradeTable->FindRow<FRGGeneralUpgradeRow>(RowName, ContextString);
		if (!Row)
		{
			continue;
		}
		// 예외사항: 최대 중첩에 도달한 후보 제외
		if (GetUpgradeStackCount(RowName) >= Row->MaxStack)
		{
			continue;
		}

		ValidRowNames.Add(RowName);
	}

	// 무작위로 섞은 뒤 앞에서 Count개만 뽑음 (중복 없이)
	for (int32 i = ValidRowNames.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		ValidRowNames.Swap(i, j);
	}

	const int32 PickCount = FMath::Min(Count, ValidRowNames.Num());
	for (int32 i = 0; i < PickCount; ++i)
	{
		const FRGGeneralUpgradeRow* Row = GeneralUpgradeTable->FindRow<FRGGeneralUpgradeRow>(ValidRowNames[i], ContextString);
		if (!Row)
		{
			continue;
		}

		FRGUpgradeOption Option;
		Option.UpgradeId = ValidRowNames[i];
		Option.CategoryText = Row->CategoryText;
		Option.NameText = Row->NameText;
		Option.DescriptionText = Row->DescriptionText;
		Option.ValueChangeText = Row->ValueChangeText;
		Result.Add(Option);
	}

	UE_LOG(LogTemp, Log, TEXT("[Progression] 강화 후보 %d개 생성 완료 (유효 후보 %d개 중)"), Result.Num(), ValidRowNames.Num());
	return Result;
}

//UI에서 카드 띄우는 함수
void URGProgressionSubsystem::PresentNextPendingLevelUpIfAny()
{
	if (bIsPresentingUpgradeChoice)
	{
		return; // 이미 카드 선택 화면이 떠 있으면 중복으로 또 안 띄움
	}
	if (PendingLevelUpCount <= 0)
	{
		return; // 대기 중인 레벨업 없음
	}
	UE_LOG(LogTemp, Warning, TEXT("PresentNextPendingLevelUpIfAny 호출됨"));
	// 카드 몇 장을 보여줄지는 "지금 막 올라간 그 레벨"의 데이터 기준
	const FRGEXPLevelRow* Row = FindCurrentLevelRow();
	const int32 ChoiceCount = Row ? Row->UpgradeChoiceCount : 3;
	//실제 후보를 뽑는 코드
	LastPresentedOptions = GenerateUpgradeOptions(ChoiceCount);
	//업글중임.
	bIsPresentingUpgradeChoice = true;
	UE_LOG(LogTemp, Warning, TEXT("OnLevelUpReady Broadcast!"));
	OnLevelUpReady.Broadcast(LastPresentedOptions);
}

void URGProgressionSubsystem::ApplyUpgradeByIndex(int32 CardIndex)
{
	if (!bIsPresentingUpgradeChoice)
	{
		return;
	}
	if (!LastPresentedOptions.IsValidIndex(CardIndex))
	{
		return;
	}

	ApplyUpgrade(LastPresentedOptions[CardIndex].UpgradeId);
}

void URGProgressionSubsystem::ApplyUpgrade(FName UpgradeId)
{
	if (!bIsPresentingUpgradeChoice)
	{
		// 카드 선택 요청도 안 했는데 결과가 들어오면 무시 (방어 코드)
		return;
	}

	// 스택 1 증가. 최종 수치 계산은 여기서 안 하고, "몇 번 골랐는지"만 기록.
	// -> 이래야 어떤 순서로 골라도 최종 스택 수만 같으면 항상 같은 결과가 나옴.
	int32& StackCount = UpgradeStacks.FindOrAdd(UpgradeId);
	StackCount += 1;

	UE_LOG(LogTemp, Warning, TEXT("[Upgrade] %s 적용됨 (현재 Stack: %d)"), *UpgradeId.ToString(), StackCount);
	OnUpgradeApplied.Broadcast(UpgradeId, StackCount);

	PendingLevelUpCount = FMath::Max(0, PendingLevelUpCount - 1);
	bIsPresentingUpgradeChoice = false;

	// 대기 중이던 다음 레벨업이 있으면 순차적으로 이어서 띄움
	PresentNextPendingLevelUpIfAny();
}

int32 URGProgressionSubsystem::GetUpgradeStackCount(FName UpgradeId) const
{
	if (const int32* Found = UpgradeStacks.Find(UpgradeId))
	{
		return *Found;
	}
	return 0;
}

float URGProgressionSubsystem::GetUpgradeEffectAmount(FName UpgradeId) const
{
	if (!GeneralUpgradeTable)
	{
		return 0.f;
	}
	if (const FRGGeneralUpgradeRow* Row = GeneralUpgradeTable->FindRow<FRGGeneralUpgradeRow>(UpgradeId, TEXT("EffectAmountLookup")))
	{
		return Row->EffectAmountPerStack;
	}
	return 0.f;
}

void URGProgressionSubsystem::ResetRun()
{
	CurrentLevel = 1;
	CurrentExperience = 0.f;
	PendingLevelUpCount = 0;
	bIsPresentingUpgradeChoice = false;
	UpgradeStacks.Empty();

	ActiveCoreUpgrades.Empty();   
	bIsPresentingCoreUpgradeChoice = false;

	const FRGEXPLevelRow* Row = FindCurrentLevelRow();
	const float RequiredExperience = Row ? Row->RequiredExperience : 0.f;
	OnExperienceChanged.Broadcast(CurrentExperience, RequiredExperience, CurrentLevel);
}

ARGBaseWeapon* URGProgressionSubsystem::GetCurrentEquippedWeapon() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ARGCharacter* PlayerCharacter = Cast<ARGCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (!PlayerCharacter)
	{
		return nullptr;
	}

	return PlayerCharacter->GetCurrentWeapon();
}

// =================================================================================================

// ==========================핵심 강화 함수 ==============================================

// ActiveCoreUpgrades 는 TArray<FName> 형.
bool URGProgressionSubsystem::HasCoreUpgrade(FName UpgradeId) const
{
	return ActiveCoreUpgrades.Contains(UpgradeId);
}
// 활성화된 업그레이드가 MaxCoreUpgradeCount(2개 예정) 보다 작으면 아직 업글 가능.
bool URGProgressionSubsystem::CanAcquireMoreCoreUpgrades() const
{
	return ActiveCoreUpgrades.Num() < MaxCoreUpgradeCount;
}

TArray<FRGUpgradeOption> URGProgressionSubsystem::GenerateCoreUpgradeOptions(int32 Count) const {
	TArray<FRGUpgradeOption> Result;

	if (!CoreUpgradeTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[CoreUpgrade] CoreUpgradeTable이 nullptr! BP_RGGameModeBase 연결 확인 필요"));
		return Result;
	}
	if (!CanAcquireMoreCoreUpgrades())
	{
		return Result;
	}

	ARGBaseWeapon* EquippedWeapon = GetCurrentEquippedWeapon();
	UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] 현재 장착 무기: %s"), EquippedWeapon ? *EquippedWeapon->GetClass()->GetName() : TEXT("nullptr"));

	TArray<FName> ValidRowNames;
	static const FString ContextString(TEXT("CoreUpgradeCandidateGen"));

	UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] 데이터테이블 총 행 개수: %d"), CoreUpgradeTable->GetRowNames().Num());

	for (const FName& RowName : CoreUpgradeTable->GetRowNames()) {
		if (ActiveCoreUpgrades.Contains(RowName)) {
			continue;
		}
		const FRGCoreUpgradeRow* Row = CoreUpgradeTable->FindRow<FRGCoreUpgradeRow>(RowName, ContextString);
		if (!Row) { continue; }

		if (Row->RequiredWeaponClass) {
			UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] %s의 필요무기: %s"), *RowName.ToString(), *Row->RequiredWeaponClass->GetName());
			if (!EquippedWeapon || !EquippedWeapon->IsA(Row->RequiredWeaponClass)) {
				UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] %s 필터에서 제외됨"), *RowName.ToString());
				continue;
			}
		}
		ValidRowNames.Add(RowName);
	}

	UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] 최종 유효 후보 개수: %d"), ValidRowNames.Num());
	//섞어
	for (int32 i = ValidRowNames.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		ValidRowNames.Swap(i, j);
	}

	const int32 PickCount = FMath::Min(Count, ValidRowNames.Num());
	for (int32 i = 0; i < PickCount; ++i)
	{
		const FRGCoreUpgradeRow* Row = CoreUpgradeTable->FindRow<FRGCoreUpgradeRow>(ValidRowNames[i], ContextString);
		if (!Row) continue;
		//실제 카드 데이터로 변환
		FRGUpgradeOption Option;
		Option.UpgradeId = ValidRowNames[i];
		Option.CategoryText = Row->CategoryText;
		Option.NameText = Row->NameText;
		Option.DescriptionText = Row->DescriptionText;
		Option.ValueChangeText = Row->ValueChangeText;
		Result.Add(Option);
	}
	return Result;
}

void URGProgressionSubsystem::PresentCoreUpgradeChoice()
{
	if (!CanAcquireMoreCoreUpgrades())
	{
		return;   // 이미 2개 다 채웠으면 맵 넘어가도 카드 안 뜸
	}

	TArray<FRGUpgradeOption> Options = GenerateCoreUpgradeOptions(3);
	if (Options.Num() == 0)
	{
		return;   // 이 무기로 고를 수 있는 핵심 강화가 더 없음
	}

	LastPresentedCoreOptions = Options;
	bIsPresentingCoreUpgradeChoice = true;
	OnCoreUpgradeReady.Broadcast(Options);   // 일반 강화와 다른 델리게이트 -> UI에서 다르게 렌더링
}

void URGProgressionSubsystem::ApplyCoreUpgrade(FName UpgradeId)
{
	if (!bIsPresentingCoreUpgradeChoice || !CanAcquireMoreCoreUpgrades())
	{
		return;
	}
	if (ActiveCoreUpgrades.Contains(UpgradeId))
	{
		return;   // 방어 코드: 중복 획득 방지
	}

	ActiveCoreUpgrades.Add(UpgradeId);
	bIsPresentingCoreUpgradeChoice = false;

	UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] %s 획득 (%d/%d)"), *UpgradeId.ToString(), ActiveCoreUpgrades.Num(), MaxCoreUpgradeCount);
}

const FRGCoreUpgradeRow* URGProgressionSubsystem::FindCoreUpgradeRow(FName UpgradeId) const
{
	if (!CoreUpgradeTable)
	{
		return nullptr;
	}
	return CoreUpgradeTable->FindRow<FRGCoreUpgradeRow>(UpgradeId, TEXT("CoreUpgradeEffectLookup"));
}

void URGProgressionSubsystem::DebugForceCoreUpgradeChoice()
{
	UE_LOG(LogTemp, Warning, TEXT("DebugForceCoreUpgradeChoice 호출됨"));
	PresentCoreUpgradeChoice();

	if (LastPresentedCoreOptions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade] 후보 없음 (이미 %d/%d 보유 중이거나 이 무기로 고를 강화가 없음)"), ActiveCoreUpgrades.Num(), MaxCoreUpgradeCount);
		return;
	}

	for (const FRGUpgradeOption& Option : LastPresentedCoreOptions)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoreUpgrade 후보] %s"), *Option.UpgradeId.ToString());
	}
}

void URGProgressionSubsystem::DebugApplyCoreUpgradeByName(const FString& UpgradeName)
{
	ApplyCoreUpgrade(FName(*UpgradeName));
}
