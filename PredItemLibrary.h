// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffect.h"

#include "PredItemLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PredItemLog, Log, All);
DECLARE_STATS_GROUP(TEXT("PredItem"), STATGROUP_PredItem, STATCAT_Advanced);

class AActor;
class UTexture2D;
class APredItemService;
class UPredInventoryComponent;
class UPredItem;
class UDataTable; 

/**
 * Static library used to interface with the item system.
 */
UCLASS()
class PREDECESSOR_API UPredItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	static const FPrimaryAssetType PredItemAssetType;

    static const FName ItemShopOpenRowName;
    static const FName ItemShopCloseRowName;
    static const FName ItemShopSelectRowName;
    static const FName ItemShopBuyItemRowName;
    static const FName ItemShopSellItemRowName;

    /**
    * Generates a debug string for the InventoryOwner, printing out the current contents of @InventoryOwners associated inventory.
    */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static void GenerateInventoryDebugString(AActor* InventoryOwner, FString& OutDebugString);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = "PredItemLibrary")
    static APredItemService* GetItemService(UObject* WorldContextObject);

    /**
     * Grabs the inventory component from an actor
     */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static UPredInventoryComponent* GetInventoryComponent(AActor* Actor);

    /**
     * Returns all loaded items in the world.
     */
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = "PredItemLibrary")
    static void GetItems(UObject* WorldContextObject, TArray<UPredItem*>& Items);

    /**
     * Returns an item with the name @ItemName. ItemName is designated by the asset name, if the data table is named "Test", then ItemName should be "Test".
     */
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = "PredItemLibrary")
    static UPredItem* GetItemByName(UObject* WorldContextObject, const FString& ItemName);

    /**
     * Returns true if @Actor has the item @Item.
     */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static bool HasItem(AActor* Actor, const UPredItem* Item);

    /**
     * Places the amount of currency that @Actor needs in order to purchase @Item in @OutCost.
     * Returns false if the item cost could not be calculated.
     */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static bool CalculateItemCostFor(AActor* Actor, UPredItem* Item, float& OutCost);

    /**
    * Returns true if @Actor can purchase @Item. Optionally skips location checks (eg, am I in range of a store).
    */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static bool CanPurchaseItem(AActor* Actor, UPredItem* Item, bool bUseLocation);

    /**
     * Returns true if @Actor can sell the item at @SlotID.
     */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static bool CanSellAtInventorySlot(AActor* Actor, int32 SlotID);

    /**
     * Places the items that @Item builds in to in @OutBuildsIntoItems.
     */
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = "PredItemLibrary")
    static void GetBuildsIntoItemsFor(UPredItem* Item, TArray<UPredItem*>& OutBuildsIntoItems);
	
    /**
     * Return the data table to pull stats from.
     */
    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static UDataTable* GetItemShopSoundData();

    //////////////////////////////////////////////////////////////////////////
    // Structs & Supporting types
    //////////////////////////////////////////////////////////////////////////

    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static float GetMagnitudeFromPredItemMagnitude(const FPredItemMagnitude& ItemMagnitude);

    UFUNCTION(BlueprintPure, Category = "PredItemLibrary")
    static float GetMagnitudeFromPredItemAttributeModifier(const FPredItemAttributeModifier& ItemAttributeMod);
};
