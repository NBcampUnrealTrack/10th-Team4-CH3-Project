// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Crafting/RGCraftingStationWidget.h"
#include "UI/View/Crafting/RGRecipeEntryWidget.h"
#include "Data/RGCraftingData.h"
#include "Components/VerticalBox.h"
#include "Engine/DataTable.h"
#include "Components/UniformGridPanel.h"

void URGCraftingStationWidget::NativeConstruct()
{
	UE_LOG(LogTemp, Warning, TEXT("[CraftingStation] NativeConstruct Called"));

	Super::NativeConstruct();

	RefreshRecipeList();
}

void URGCraftingStationWidget::RefreshInventorySlots()
{
	if (!UniformGrid_Inventory || !InventorySlotClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[CraftingStation] Inventory UI reference is missing")
		);

		return;
	}

	UniformGrid_Inventory->ClearChildren();

	const int32 ColumnCount = 5;

	for (int32 Index = 0; Index < InventoryItems.Num(); ++Index)
	{
		const FRGInventorySlotData& SlotData = InventoryItems[Index];

		URGInventorySlotWidget* SlotWidget = CreateWidget<URGInventorySlotWidget>(
				this,
				InventorySlotClass
			);

		if (!SlotWidget)
		{
			continue;
		}

		UTexture2D* ItemIcon = nullptr;

		if (ItemConfigTable)
		{
			const FRGItemConfigRow* ItemConfig = ItemConfigTable->FindRow<FRGItemConfigRow>(
					SlotData.ItemID,
					TEXT("InventorySlot")
				);

			if (ItemConfig)
			{
				ItemIcon = ItemConfig->Icon.LoadSynchronous();
			}
		}

		SlotWidget->SetInventorySlotData(
			SlotData.ItemID,
			SlotData.ItemCount,
			ItemIcon,
			SlotData.bSelected
		);

		const int32 Row = Index / ColumnCount;
		const int32 Column = Index % ColumnCount;

		UniformGrid_Inventory->AddChildToUniformGrid(
			SlotWidget,
			Row,
			Column
		);
	}
}

void URGCraftingStationWidget::RefreshRecipeList()
{
	if (!VerticalBox_RecipeList || !CraftingRecipeTable || !RecipeEntryClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[CraftingStation] Table=%s / EntryClass=%s / RecipeList=%s"),
			*GetNameSafe(CraftingRecipeTable),
			*GetNameSafe(RecipeEntryClass),
			*GetNameSafe(VerticalBox_RecipeList));

		return;
	}

	VerticalBox_RecipeList->ClearChildren();

	TArray<FRGCraftingRecipeRow*> RecipeRows;
	CraftingRecipeTable->GetAllRows(TEXT("CraftingRecipeList"), RecipeRows);

	UE_LOG(LogTemp, Warning,
		TEXT("[CraftingStation] RecipeRows Num=%d"),
		RecipeRows.Num());

	for (const FRGCraftingRecipeRow* RecipeRow : RecipeRows)
	{
		if (!RecipeRow)
		{
			continue;
		}

		TArray<FText> MaterialNames;
		TArray<int32> MaterialAmounts;

		for (const FRGRecipeMaterial& Material : RecipeRow->RequiredMaterials)
		{
			MaterialNames.Add(FText::FromName(Material.ItemID));
			MaterialAmounts.Add(Material.RequiredAmount);
		}

		URGRecipeEntryWidget* RecipeEntry = CreateWidget<URGRecipeEntryWidget>(this, RecipeEntryClass);
		
		UE_LOG(LogTemp, Warning,
			TEXT("[CraftingStation] RecipeWidget=%s"),
			*GetNameSafe(RecipeEntry));

		if (!RecipeEntry)
		{
			continue;
		}

		RecipeEntry->SetRecipeDisplayData(
			FText::FromName(RecipeRow->ResultItemID),
			FText::FromString(TEXT("CRAFTABLE MODULE")),
			MaterialNames,
			MaterialAmounts
		);

		VerticalBox_RecipeList->AddChild(RecipeEntry);

		UE_LOG(LogTemp, Warning,
			TEXT("[CraftingStation] RecipeWidget Added"));
	}
}

void URGCraftingStationWidget::SetInventoryItems(const TArray<FRGInventorySlotData>& InItems)
{
	InventoryItems = InItems;
	RefreshInventorySlots();
}


