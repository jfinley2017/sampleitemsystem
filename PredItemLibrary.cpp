// Fill out your copyright notice in the Description page of Project Settings.


#include "PredItemLibrary.h"
#include "Lib/PredLoggingLibrary.h"
#include "Engine/DataTable.h"
#include "Services/PredItemService.h"
#include "BasePredecessorGameState.h"
#include "Component/PredInventoryComponent.h"
#include "PredBlueprintFunctionLibrary.h"

DEFINE_LOG_CATEGORY(PredItemLog);

const FPrimaryAssetType UPredItemLibrary::PredItemAssetType = TEXT("PredItem");

const FName UPredItemLibrary::ItemShopBuyItemRowName = "ItemShop_BuyItem";
const FName UPredItemLibrary::ItemShopSellItemRowName = "ItemShop_SellItem";
const FName UPredItemLibrary::ItemShopOpenRowName = "ItemShop_Open";
const FName UPredItemLibrary::ItemShopCloseRowName = "ItemShop_Close";
const FName UPredItemLibrary::ItemShopSelectRowName = "ItemShop_SelectItem";

APredItemService* UPredItemLibrary::GetItemService(UObject* WorldContextObject)
{
    ABasePredecessorGameState* GameState = WorldContextObject->GetWorld()->GetGameState<ABasePredecessorGameState>();
    if (!GameState)
    {
        static bool bPrintWrongGSError = true;
        if (bPrintWrongGSError)
        {
            TRACESTATIC(PredItemLog, Error, "PredGameState not found. Could not reference ItemService.")
        }
        return nullptr;
    }
    return GameState->GetItemService();
}

UPredInventoryComponent* UPredItemLibrary::GetInventoryComponent(AActor* Actor)
{
    if (!Actor) { return nullptr; }

    return Actor->FindComponentByClass<UPredInventoryComponent>();
}

void UPredItemLibrary::GetItems(UObject* WorldContextObject, TArray<UPredItem*>& Items)
{
    APredItemService* ItemService = GetItemService(WorldContextObject);
    if (ItemService)
    {
        return ItemService->GetItems(Items);
    }
}

UPredItem* UPredItemLibrary::GetItemByName(UObject* WorldContextObject, const FString& ItemName)
{
    APredItemService* ItemService = GetItemService(WorldContextObject);
    if (ItemService)
    {
        UPredItem* FoundItem = ItemService->GetItemFromName(ItemName);
        //#TODO move logging into item service
        if (!FoundItem) { TRACESTATIC(PredItemLog, Warning, "Attempted to get item %s but an item with name %s did not exist.", *ItemName, *ItemName) }
        return FoundItem;
    }
    return nullptr;
}

bool UPredItemLibrary::HasItem(AActor* Actor, const UPredItem* Item)
{
    UPredInventoryComponent* InventoryComponent = GetInventoryComponent(Actor);
    if (InventoryComponent)
    {
        return InventoryComponent->HasItem(Item);
    }
    return false;
}

void UPredItemLibrary::GenerateInventoryDebugString(AActor* InventoryOwner, FString& OutDebugString)
{
    if (!InventoryOwner) { return; }

    UPredInventoryComponent* InventoryComponent = InventoryOwner->FindComponentByClass<UPredInventoryComponent>();
    if (InventoryComponent)
    {
        InventoryComponent->GenerateDebugString(OutDebugString);
        return;
    }

    OutDebugString = FString::Printf(TEXT("No inventory component for %s"), *GetNameSafe(InventoryOwner));
}

bool UPredItemLibrary::CalculateItemCostFor(AActor* Actor, UPredItem* Item, float& OutCost)
{
    UPredInventoryComponent* InventoryComponent = GetInventoryComponent(Actor);
    if (InventoryComponent)
    {
        OutCost = InventoryComponent->CalculateItemCost(Item);
        return true;
    }
    return false;
}

bool UPredItemLibrary::CanPurchaseItem(AActor* Actor, UPredItem* Item, bool bUseLocation)
{
    UPredInventoryComponent* InventoryComponent = GetInventoryComponent(Actor);
    if (InventoryComponent)
    {
        return InventoryComponent->CanPurchaseItem(Item, bUseLocation);
    }

    return false;
}

bool UPredItemLibrary::CanSellAtInventorySlot(AActor* Actor, int32 SlotID)
{
    UPredInventoryComponent* InventoryComponent = GetInventoryComponent(Actor);
    if (InventoryComponent)
    {
        return InventoryComponent->CanSellAtInventorySlot(SlotID);
    }
    return false;
}

void UPredItemLibrary::GetBuildsIntoItemsFor(UPredItem* Item, TArray<UPredItem*>& OutBuildsIntoItems)
{
    if (!Item)
    {
        return;
    }

    OutBuildsIntoItems = Item->BuildsInto;
}

UDataTable* UPredItemLibrary::GetItemShopSoundData()
{
    return UPredBlueprintFunctionLibrary::GetGlobalDataSingleton()->GlobalSoundTable;
}

float UPredItemLibrary::GetMagnitudeFromPredItemMagnitude(const FPredItemMagnitude& ItemMagnitude)
{
    return ItemMagnitude.GetMagnitude();
}

float UPredItemLibrary::GetMagnitudeFromPredItemAttributeModifier(const FPredItemAttributeModifier& ItemAttributeMod)
{
    return ItemAttributeMod.GetMagnitude();
}

