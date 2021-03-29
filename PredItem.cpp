// Fill out your copyright notice in the Description page of Project Settings.

#include "PredItem.h"
#include "PredInventoryComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BaseAttributeSet.h"
#include "PredItemLibrary.h"

FPrimaryAssetId UPredItem::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(UPredItemLibrary::PredItemAssetType, GetFName());
}

FString UPredItem::GetIdentifierString() const
{
    return GetPrimaryAssetId().ToString();
}

FString UPredItem::GetItemName() const
{
    return GetFName().ToString();
}

float UPredItem::GetTotalItemCost() const
{
    // If we've already calculated the cost for this item, return that
    if (HasCachedTotalItemCost)
    {
        return CachedTotalItemCost;
    }

    // Iterate through this item's children
    float ReturnedCost = GetItemCost();
    for (UPredItem* RequiredItem : RequiredItems)
    {
        ReturnedCost += RequiredItem->GetTotalItemCost();
    }

    // Set the cached value for any future queries.
    CachedTotalItemCost = ReturnedCost;
    HasCachedTotalItemCost = true;

    return ReturnedCost;
}

float UPredItem::GetItemCost() const
{
    return Price.GetMagnitude();
}

bool UPredItem::CanPurchase(UPredInventoryComponent* InventoryComponent)
{
    // refactored because CanAfford now takes into account items that exist in the passed in InventoryComponent.
    return CanAfford(InventoryComponent);
}

bool UPredItem::CanAfford(UPredInventoryComponent* InventoryComponent)
{
    bool bFoundAttribute = false;
    float GoldAmount = UAbilitySystemBlueprintLibrary::GetFloatAttribute(InventoryComponent->GetOwner()
                                                                         , UBaseAttributeSet::GetGoldAttribute(), bFoundAttribute);
    return bFoundAttribute && (GoldAmount >= GetItemCostFor(InventoryComponent));
}

float UPredItem::GetItemCostFor(UPredInventoryComponent* InventoryComponent)
{
    TArray<FPredInventorySlot> Inventory;
    InventoryComponent->GetAllInventorySlots(Inventory);

    TArray<const UPredItem*> InventoryItemDefinitions;
    for (FPredInventorySlot& InventorySlot : Inventory)
    {
        if (InventorySlot.IsEmpty())
        {
            continue;
        }

        InventoryItemDefinitions.Add(InventorySlot.SlottedItem.Item);
    }

    return GetItemCostForHelper(InventoryItemDefinitions);
}

float UPredItem::GetItemCostForHelper(TArray<const UPredItem*>& RemainingInventory)
{
    // To buy this item, you need the base price. Always.
    float ReturnedCost = GetItemCost();

    // Try the children, skipping ones which we already have in our inventory.
    for (UPredItem* Item : RequiredItems)
    {
        // If we contain the child, remove it so that we don't process again and don't bump up the price.
        if (RemainingInventory.Contains(Item))
        {
            RemainingInventory.RemoveSingle(Item);
            continue;
        }

        ReturnedCost += Item->GetItemCostForHelper(RemainingInventory);
    }

    return ReturnedCost;
}
