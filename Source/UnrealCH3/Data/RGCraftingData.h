#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "RGCraftingData.generated.h"

UENUM(BlueprintType)
enum class ERGItemType : uint8
{
	Material,
	Weapon,
	Module,
	Consumable
};

UENUM(BlueprintType)
enum class ERGSocketType : uint8
{
	Core,
	Barrel,
	Utility
};

UENUM(BlueprintType)
enum class ERGModuleEffectType : uint8
{
	Damage,
	FireRate,
	Magazine,
	ReloadSpeed,
	ChargeSpeed
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FRGItemConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERGItemType ItemType = ERGItemType::Material;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxStack = 99;
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FRGRecipeMaterial
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 RequiredAmount = 1;
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FRGCraftingRecipeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName RecipeID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ResultItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FRGRecipeMaterial> RequiredMaterials;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float CraftTime = 0.0f;
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FRGSocketConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName SocketID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERGSocketType SocketType = ERGSocketType::Core;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AllowedWeaponID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName DefaultModuleID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsUnlocked = true;
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FRGModuleConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ModuleID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERGModuleEffectType EffectType = ERGModuleEffectType::Damage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float EffectValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct UNREALCH3_API FRGInventorySlotData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ItemCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSelected = false;
};