#include "UI/View/Menu/PauseMenuWidget.h"
#include "Components/Button.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Resume))
	{
		Button_Resume->OnClicked.AddUniqueDynamic(
			this,
			&UPauseMenuWidget::HandleResumeClicked);
	}
}

void UPauseMenuWidget::NativeDestruct()
{
	if (IsValid(Button_Resume))
	{
		Button_Resume->OnClicked.RemoveDynamic(
			this,
			&UPauseMenuWidget::HandleResumeClicked);
	}

	Super::NativeDestruct();
}

void UPauseMenuWidget::HandleResumeClicked()
{
	OnResumeRequested.Broadcast();
}