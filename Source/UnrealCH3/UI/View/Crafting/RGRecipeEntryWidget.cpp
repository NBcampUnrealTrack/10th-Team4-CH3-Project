// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Crafting/RGRecipeEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void URGRecipeEntryWidget::SetRecipeDisplayData(const FText& InRecipeName, const FText& InDescription, const TArray<FText>& InMaterialNames, const TArray<int32>& InMaterialAmounts)
{
	if (Text_RecipeName)
	{
		Text_RecipeName->SetText(InRecipeName);
	}

	if (Text_RecipeDescription)
	{
		Text_RecipeDescription->SetText(InDescription);
	}

	if (Text_RecipeMaterials)
	{
		Text_RecipeMaterials->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_MaterialName_1 && Text_MaterialCount_1)
	{
		if (InMaterialNames.Num() > 0 && InMaterialAmounts.Num() > 0)
		{
			Text_MaterialName_1->SetText(InMaterialNames[0]);
			Text_MaterialCount_1->SetText(FText::AsNumber(InMaterialAmounts[0]));
			Text_MaterialName_1->SetVisibility(ESlateVisibility::Visible);
			Text_MaterialCount_1->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Text_MaterialName_1->SetVisibility(ESlateVisibility::Collapsed);
			Text_MaterialCount_1->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_MaterialName_2 && Text_MaterialCount_2)
	{
		if (InMaterialNames.Num() > 1 && InMaterialAmounts.Num() > 1)
		{
			Text_MaterialName_2->SetText(InMaterialNames[1]);
			Text_MaterialCount_2->SetText(FText::AsNumber(InMaterialAmounts[1]));
			Text_MaterialName_2->SetVisibility(ESlateVisibility::Visible);
			Text_MaterialCount_2->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Text_MaterialName_2->SetVisibility(ESlateVisibility::Collapsed);
			Text_MaterialCount_2->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void URGRecipeEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Craft)
	{
		Button_Craft->OnClicked.AddDynamic(this,&URGRecipeEntryWidget::HandleCraftButtonClicked);
	}

}

void URGRecipeEntryWidget::HandleCraftButtonClicked()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RecipeEntry] Craft button clicked: %s"),
		*GetNameSafe(this)
	);
}

