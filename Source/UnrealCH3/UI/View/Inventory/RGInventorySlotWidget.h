// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGInventorySlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * 
 */
UCLASS()
class UNREALCH3_API URGInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetInventorySlotData(FName InItemID, int32 InItemCount, UTexture2D* InItemIcon, bool bInSelected);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_ItemSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_SelectedFrame;


};
