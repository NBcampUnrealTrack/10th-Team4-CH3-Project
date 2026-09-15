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

//UI에서 카드 띄우는 함수 ( 수정 예정)
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

	bIsPresentingUpgradeChoice = true;
	OnLevelUpReady.Broadcast(CurrentLevel, ChoiceCount);
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