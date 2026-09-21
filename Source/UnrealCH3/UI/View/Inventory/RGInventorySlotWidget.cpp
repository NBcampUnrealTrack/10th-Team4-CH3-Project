// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/View/Inventory/RGInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Components/UniformGridPanel.h"
#include "Engine/DataTable.h"

void URGInventorySlotWidget::SetInventorySlotData(FName InItemID, int32 InItemCount, UTexture2D* InItemIcon, bool bInSelected)
{
	if (Image_ItemIcon)
	{
		if (InItemIcon)
		{
			Image_ItemIcon->SetBrushFromTexture(InItemIcon);
			Image_ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_ItemCount)
	{
		if (InItemCount > 0)
		{
			Text_ItemCount->SetText(FText::AsNumber(InItemCount));
			Text_ItemCount->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Text_ItemCount->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Image_SelectedFrame)
	{
		Image_SelectedFrame->SetVisibility(bInSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
