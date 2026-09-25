// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGRecipeEntryWidget.generated.h"

class UTextBlock;
class UButton;
/**
 * 
 */
UCLASS()
class UNREALCH3_API URGRecipeEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Controller 가 레시피 표시 정보를 전달할때 호출
	UFUNCTION(BlueprintCallable)
	void SetRecipeDisplayData(const FText& InRecipeName, const FText& InDescription, const TArray<FText>& InMaterialNames, const TArray<int32>& InMaterialAmounts);
	
protected:

	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleCraftButtonClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RecipeName;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RecipeDescription;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RecipeMaterials;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MaterialName_1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MaterialCount_1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MaterialName_2;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MaterialCount_2;
	
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UButton> Button_Craft;

};
