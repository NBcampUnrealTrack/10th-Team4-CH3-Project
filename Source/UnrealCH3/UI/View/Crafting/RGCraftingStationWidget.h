// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/View/Inventory/RGInventorySlotWidget.h"
#include "Data/RGCraftingData.h"
#include "RGCraftingStationWidget.generated.h"

class UDataTable;
class UVerticalBox;
class URGRecipeEntryWidget;
class URGInventorySlotWidget;
class UUniformGridPanel;


/**
 * 
 */
UCLASS()
class UNREALCH3_API URGCraftingStationWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	void RefreshInventorySlots();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UDataTable> CraftingRecipeTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TSubclassOf<URGRecipeEntryWidget> RecipeEntryClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_RecipeList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<URGInventorySlotWidget> InventorySlotClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> UniformGrid_Inventory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UDataTable> ItemConfigTable;

	UPROPERTY()
	TArray<FRGInventorySlotData> InventoryItems;

public:
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void RefreshRecipeList();

	UFUNCTION(BlueprintCallable)
	void SetInventoryItems(const TArray<FRGInventorySlotData>& InItems);

};
