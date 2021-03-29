#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "PredItemService.generated.h"

class UPredItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemsLoadedSignature);

/**
 * Handles loading and retrieving of items.
 */
UCLASS()
class PREDECESSOR_API APredItemService : public AInfo
{
	GENERATED_BODY()
	
public:

    APredItemService();

    UPROPERTY(BlueprintAssignable, Category = "PredItem")
    FOnItemsLoadedSignature OnItemsLoaded;

    // AInfo
    virtual void PreInitializeComponents() override;
    // ~AInfo

    /** Retrieves all loaded items, sorted by price */
    UFUNCTION(BlueprintPure, Category = "PredItem")
    void GetItems(TArray<UPredItem*>& OutItems);

    /** Returns an item designated by ItemName, where ItemName is the asset name of the item */
    UFUNCTION(BlueprintPure, Category = "PredItem")
    UPredItem* GetItemFromName(const FString& ItemName);

    /** Returns an item from a FPrimaryAssetID */
    UFUNCTION(BlueprintPure, Category = "PredItem")
    UPredItem* GetItemFromPrimaryID(FPrimaryAssetId AssetID);

protected:

    // AInfo
    virtual void BeginPlay() override;
    // ~AInfo

    /** Map containing all items loaded, only valid on server */
    UPROPERTY()
    TMap<FPrimaryAssetId, UPredItem*> LoadedItems;

    // TODO: investigate replicating FPrimaryAssetIds and not these pointers, need to change how buying/selling works in order to achieve this.
    UPROPERTY(ReplicatedUsing=OnRep_SortedItems)
    TArray<UPredItem*> SortedItems;

    UFUNCTION()
    void Internal_NotifyItemsLoaded();

    UFUNCTION()
    void OnRep_SortedItems();

};
