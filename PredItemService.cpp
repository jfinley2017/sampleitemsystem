
#include "PredItemService.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"

#include "PredItemLibrary.h"
#include "PredLoggingLibrary.h"
#include "PredItem.h"

APredItemService::APredItemService()
{
    SetReplicates(true);
    bAlwaysRelevant = true;
}

void APredItemService::BeginPlay()
{
    Super::BeginPlay();


}

void APredItemService::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APredItemService, SortedItems);
}

void APredItemService::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    if (HasAuthority())
    {
        UAssetManager* AssetManager = GEngine->AssetManager;
        TArray<FPrimaryAssetId> PrimaryAsset;
        AssetManager->GetPrimaryAssetIdList(UPredItemLibrary::PredItemAssetType, PrimaryAsset);
        FStreamableDelegate ItemsLoadedDelegate;
        ItemsLoadedDelegate.BindUFunction(this, "Internal_NotifyItemsLoaded");
        AssetManager->LoadPrimaryAssets(PrimaryAsset, TArray<FName>(), ItemsLoadedDelegate);
    }
    
}

void APredItemService::GetItems(TArray<UPredItem*>& OutItems)
{
    OutItems = SortedItems;
}

UPredItem* APredItemService::GetItemFromName(const FString& ItemName)
{
    return GetItemFromPrimaryID(FPrimaryAssetId(UPredItemLibrary::PredItemAssetType, FName(*ItemName)));
}

UPredItem* APredItemService::GetItemFromPrimaryID(FPrimaryAssetId AssetID)
{
    UAssetManager* AssetManager = GEngine->AssetManager;
    return AssetManager->GetPrimaryAssetObject<UPredItem>(AssetID);
}

void APredItemService::Internal_NotifyItemsLoaded()
{
    UAssetManager* AssetManager = GEngine->AssetManager;

    TArray<UObject*> Items;
    AssetManager->GetPrimaryAssetObjectList(UPredItemLibrary::PredItemAssetType, Items);
    for (UObject* Item : Items)
    {
        UPredItem* ItemAsPredItem = Cast<UPredItem>(Item);
        LoadedItems.Add(ItemAsPredItem->GetPrimaryAssetId(), ItemAsPredItem);
        SortedItems.Add(ItemAsPredItem);
        TRACE(PredItemLog, Log, "%s Loaded.", *ItemAsPredItem->GetIdentifierString());
    }

    // sort by total price
    int i, j;
    for (i = 0; i < SortedItems.Num(); i++)
    {
        for (j = 0; j < SortedItems.Num() - i - 1; j++)
        {
            if (SortedItems[j]->GetTotalItemCost() > SortedItems[j + 1]->GetTotalItemCost())
            {
                UPredItem* Temp = SortedItems[j];
                SortedItems[j] = SortedItems[j + 1];
                SortedItems[j + 1] = Temp;
            }
        }
    }

    // setup the builds-into sections.
    for (UPredItem* Item : SortedItems)
    {
        for (UPredItem* RequiredItem : Item->RequiredItems)
        {
            RequiredItem->BuildsInto.AddUnique(Item);
        }
    }

    TRACE(PredItemLog, Log, "Items loaded.");
    OnItemsLoaded.Broadcast();
}

void APredItemService::OnRep_SortedItems()
{
    OnItemsLoaded.Broadcast();
}
