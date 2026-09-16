// Fill out your copyright notice in the Description page of Project Settings.
#include "RGProgressionSubsystem.h"
#include "RGEXPLevelRow.h"
#include "Engine/DataTable.h"

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

	// 카드 몇 장을 보여줄지는 "지금 막 올라간 그 레벨"의 데이터 기준
	const FRGEXPLevelRow* Row = FindCurrentLevelRow();
	const int32 ChoiceCount = Row ? Row->UpgradeChoiceCount : 3;
	//실제 후보를 뽑는 코드
	LastPresentedOptions = GenerateUpgradeOptions(ChoiceCount);
	//업글중임.
	bIsPresentingUpgradeChoice = true;

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

	const FRGEXPLevelRow* Row = FindCurrentLevelRow();
	const float RequiredExperience = Row ? Row->RequiredExperience : 0.f;
	OnExperienceChanged.Broadcast(CurrentExperience, RequiredExperience, CurrentLevel);
}