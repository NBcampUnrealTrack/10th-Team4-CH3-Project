#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RGPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class UNREALCH3_API ARGPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ARGPlayerController();

protected:
	virtual void BeginPlay() override;

public:

	UFUNCTION(Exec, Category = "Progression|Debug")
	void DebugForceLevelUp();

	UFUNCTION(Exec)
	void DebugForceCoreUpgradeChoice();

	UFUNCTION(Exec)
	void DebugApplyCoreUpgradeByName(const FString& UpgradeName);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultIMC;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> GrappleAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HealAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> GrenadeAction;

};
