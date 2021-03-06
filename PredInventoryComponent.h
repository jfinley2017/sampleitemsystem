// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameplayEffect.h"
#include "GameplayTags.h"

#include "PredItem.h"

#include "PredInventoryComponent.generated.h"

class UTexture2D;
class UBaseGameplayAbility;

/**
 * Represents a currently-active unique effect.
 */
USTRUCT(BlueprintType)
struct FPredActiveUniqueEffect
{

    GENERATED_BODY()

    FPredActiveUniqueEffect() {};

    FPredActiveUniqueEffect(FGameplayTag InEffectTag, FActiveGameplayEffectHandle InActiveEffectHandle) :
        EffectTag(InEffectTag),
        ActiveEffectHandle(InActiveEffectHandle) {};

    UPROPERTY()
    FGameplayTag EffectTag;

    UPROPERTY()
    FActiveGameplayEffectHandle ActiveEffectHandle;

};


/**
 * Represents an equipped item. If there are two of the same equipped items in an inventory, there will be two distinct
 * structs representing them (each with a unique ID).
 * Maintains any state about the effects an item may have applied.
 */
USTRUCT(BlueprintType)
struct FPredActiveItem
{
    GENERATED_BODY()

    FPredActiveItem() { UniqueItemID = FGuid::NewGuid(); }
    
    /** Effectively the CDO of the item. Generated by the asset manager */
    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    const UPredItem* Item = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    FGuid UniqueItemID;

    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    TArray<FActiveGameplayEffectHandle> ActiveEffects;

    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    TArray<FPredActiveUniqueEffect> ActiveUniqueEffects;

    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    FGameplayAbilitySpecHandle ActiveAbility;

    bool IsValid() { return Item != nullptr; }

};

/**
 * Represents a single slot in the inventory. 
 * Gives us a chance to visualize the slot and maintain state like InputID.
 * Good to pass around to UI.
 */
USTRUCT(BlueprintType)
struct FPredInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    int32 SlotID;

    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    FPredActiveItem SlottedItem;

    bool IsEmpty() { return !SlottedItem.IsValid(); }
    UTexture2D* GetItemIcon() { return IsEmpty() ? nullptr : SlottedItem.Item->Icon; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSlotUpdatedSignature, const FPredInventorySlot&, ItemSlot);
DECLARE_DELEGATE_OneParam(FUseInventorySlot, int32);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PREDECESSOR_API UPredInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPredInventoryComponent();

    /**
     * Fired whenever any change is made to the internal inventory.
     * Useful for refreshing UI.
     */
    UPROPERTY(BlueprintAssignable, Category = "PredInventoryComponent")
    FOnItemSlotUpdatedSignature OnItemSlotUpdated;

    /**
     * Tries to buy an item at the first slot available. Returns true if successful.
     * Returning true in this state does not mean that the item was actually equipped, we are still pending server approval.
     * However a false return is always correct and will go no further.
     */
    UFUNCTION(BlueprintCallable, Category = "PredInventoryComponent")
    bool TryBuyItem(UPredItem* Item);

    /**
     * Tries to sell an item at the specified slot. Returns true if successful.
     * Returning true in this state does not mean that the item at the slot was actually sold, we are still pending server approval.
     * However a false return is always correct and will go no further.
     */
    UFUNCTION(BlueprintCallable, Category = "PredInventoryComponent")
    bool TrySellItem(int32 SlotToSellAt);

    /**
     * Equips an item, finding the first unused item slot. Cannot be ran by clients.
     */
    UFUNCTION(BlueprintCallable, Category = "PredInventoryComponent")
    void EquipItem(UPredItem* Item, int32 Count);

    /**
     * Equips an item at the specified slot. Cannot be ran by clients.
     */
    UFUNCTION()
    void EquipItemAtSlot(UPredItem* Item, int32 Count, int32 Slot);

    /**
     * Removes the first item found that matches @Item. Cannot be ran by clients.
     */
    UFUNCTION(BlueprintCallable, Category = "PredInventoryComponent")
    void RemoveItem(const UPredItem* Item, int32 Count);

    /**
     * Removes an item at the specified slot. Cannot be ran by clients.
     */
    UFUNCTION()
    void RemoveItemAtSlot(int32 Count, int32 Slot);

    /**
     * Places the slot found at index @SlotIndex in @OutSlot. Returns false if the index is invalid (out of bounds).
     */
    UFUNCTION(BlueprintPure, Category = "PredInventory")
    bool GetInventorySlotAt(int32 SlotIndex, FPredInventorySlot& OutSlot);

    /**
     * Generates a debug string, sets OutDebugString to "" prior to appending debug string.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventory")
    void GenerateDebugString(FString& OutDebugString);

    /**
     * Returns all inventory slots.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventory")
    void GetAllInventorySlots(TArray<FPredInventorySlot>& Slots);

    /**
     * Returns the cost of the item @Item.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventory")
    float CalculateItemCost(UPredItem* Item);

    /**
     * Returns the selling price of @Item. This is usually the price of the item, reduced by some modifier.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventoryComponent")
    float GetItemSellPrice(const UPredItem* Item);

    /**
     * Returns true if this component can purchase the item. Can toggle whether or not location matters (useful for store ui)
     */
    UFUNCTION(BlueprintPure, Category = "PredInventoryComponent")
    bool CanPurchaseItem(UPredItem* Item, bool bUseLocation = true);

    /**
     * Returns true if this component can sell the item at the specified slot. False if the slot is empty.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventoryComponent")
    bool CanSellAtInventorySlot(int32 SlotID);

    /**
     * Returns the current price of the item at the specified slot.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventoryComponent")
    float GetItemPriceAtInventorySlot(int32 SlotID);

    /**
     * Returns the count of an item.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventoryComponent")
    int32 GetItemCount(const UPredItem* Item);

    /**
     * Returns true if we have at least one instance of this item.
     */
    UFUNCTION(BlueprintPure, Category = "PredInventoryComponent")
    bool HasItem(const UPredItem* Item);

    UFUNCTION()
    void TryUseInventorySlot(int32 Idx);

    UFUNCTION()
    void SetupInventoryInput(UInputComponent* InputComponent);

protected:

    // UActorComponent
    virtual void BeginPlay() override;
    // ~UActorComponent interface

    UFUNCTION()
    void SetupInventorySlots();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_TryBuyItem(UPredItem* Item);

    // TODO: use FGuids rather than int32 slots for this to allow for item moving in inventory
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_TrySellItem(int32 SlotToSellAt);

    /**
     * Finds a slot that contains the designated item, also placing the found slot in @OutItemSlot. Returns -1 if no slot was found.
     */
    UFUNCTION()
    int32 FindSlotFromItem(const UPredItem* Item, FPredInventorySlot& OutItemSlot);

    UFUNCTION()
    void ApplyItemEffectsToOwner(FPredActiveItem& Item);

    UFUNCTION()
    void RemoveItemEffectsFromOwner(FPredActiveItem& Item);

    /**
     * Returns true if we have room for the item in our inventory
     */
    UFUNCTION()
    bool HasRoomForItem(const UPredItem* Item);

    /**
     * Finds the first empty slot, placing the index in @EmptySlot
     */
    UFUNCTION()
    bool FindEmptySlot(int32& OutEmptySlot);

    /**
     * Recursively removes items after being sold.
     * Handles cases where we have partial parts of children after buying an item (at a discount, because of having children).
     */
    UFUNCTION()
    void ClearInventoryPostPurchase(const UPredItem* PurchasedItem);

    /**
     * Recursive helper for clearing an inventory post-purchase of an item.
     */
    void ClearInventoryPostPurchaseHelper(const UPredItem* ChildItem);

    /**
     * Determines if an item is providing the specified unique effect for
     */
    bool IsProviderOfUniqueEffect(const FPredActiveItem& Item, FGameplayTag UniqueEffectIdentifier);

    /**
     * Returns true if this effect is currently applied to the owner.
     */
    bool IsUniqueIdentifierApplied(const FGameplayTag& EffectIdentifier);

    /**
     * Iterates through the inventory re-applying effects if they should be applied. 
     * Used to re-apply unique effects after an item is removed (and thus, the unique effect might be removed) in the case where we have two items which want to
     * apply the same unique effect.
     */
    void RegenerateInventoryEffectsPostItemRemoval();

    int32 NumInventorySlots = 6;
    int32 NumActivateableSlots = 6;

    /**
     * How much gold should we start with? Note that this should be handled elsewhere.
     * Will be ran every time the character respawns. GameMode?
     */
    float StartingGold = 500.0f;

    /**
     * At what price will the store by the item back from us at? Calculated by TotalItemPrice() * SellModifier.
     */
    float SellModifier = .75f;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Inventory, BlueprintReadOnly, Category = "Inventory")
    TArray<FPredInventorySlot> Inventory;

    /**
     * Tracks unique effects to the provider of said effects.
     */
    TMap<FGameplayTag, FPredActiveItem> UniqueProviders;

    /**
     * Tracks granted abilities to the item that is providing the ability.
     */
    TMap<TSubclassOf<UBaseGameplayAbility>, FPredActiveItem> AbilityProvider;

    UFUNCTION()
    virtual void OnRep_Inventory();

};
