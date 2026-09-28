#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResumeRequested);

UCLASS()
class UNREALCH3_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "UI|Event")
	FOnResumeRequested OnResumeRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleResumeClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Resume;
};