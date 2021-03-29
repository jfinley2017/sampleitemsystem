// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "Engine/CurveTable.h"

#include "PredItem.generated.h"

class UBaseGameplayAbility;
class UPredInventoryComponent;

/**
 * Defines the operation to use when applying an attribute change.
 */
UENUM(BlueprintType)
enum class EPredItemAttributeModType : uint8
{
    Add, 
    Multiply,
};

UENUM(BlueprintType)
enum class EPredItemAttributeMagnitudeType : uint8
{
    NoCurve,
    Curve
};

/**
 * Allows switching between curve magnitudes and flat magnitudes. Useful for testing
 * or things we don't want hooked to data.
 *
 * Presumably will be deprecated once we have a better way of iterating on "LiveOps" (data we want edited outside of the editor) workflow.
 */
USTRUCT(BlueprintType)
struct FPredItemMagnitude
{
    GENERATED_BODY()
    
    /**
     * Designates whether the FlatMagnitude or the CurveMagnitude is used to determine magnitude.
     */
    UPROPERTY(EditDefaultsOnly, Category = "PredItemMagnitude")
    EPredItemAttributeMagnitudeType MagnitudeType;

    /**
     * A flat magnitude. Used to quickly iterate.
     */
    UPROPERTY(EditAnywhere, Category = "PredItemMagnitude")
    float FlatMagnitude;

    /**
     * A curve magnitude. Used to hookup to data.
     */
    UPROPERTY(EditAnywhere, Category = "PredItemMagnitude")
    FCurveTableRowHandle CurveMagnitude;

    float GetMagnitude() const
    {
        return MagnitudeType == EPredItemAttributeMagnitudeType::Curve ? CurveMagnitude.Eval(1.0f, "GetItemAttributeModifier") :
            FlatMagnitude;
    }

};

/**
 * Defines how we manipulate a single attribute on an item. An item can have many of these defined.
 * Saves us the trouble of having to define specific GEs for "stat sticks".
 */
USTRUCT(BlueprintType)
struct FPredItemAttributeModifier
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    EPredItemAttributeModType AttributeModType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FPredItemMagnitude Magnitude;
    
    float GetMagnitude() const
    {
        return Magnitude.GetMagnitude();
    }
};

/**
 * Defines a unique attribute modification. 
 * This attribute modifier should not be applied if another instance with the same UniqueIdentifier is already active.
 * Boots would use this structure to give a unique instance of movespeed to the buyer.
 */
USTRUCT(BlueprintType)
struct FPredUniqueItemAttributeModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FGameplayTag UniqueIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FPredItemAttributeModifier AttributeModifier;

};

/**
 * Defines a uniquely applied GameplayEffect.
 * Used for things which simple attributes cannot achieve and we are forced to create a GameplayEffect.
 * Can be used for granting other, passive abilities.
 * If another effect exists using this UniqueIdentifier, we should not be applied.
 */
USTRUCT(BlueprintType)
struct FPredUniqueItemEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FGameplayTag UniqueIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    TSubclassOf<UGameplayEffect> UniqueEffect;

};

/**
 * Data asset which defines a single item. 
 * NOT an actual item instance, rather a definition of an item. "The item with this ID has the name Dagger and gives +10 AttackSpeed".
 * Sort of like a CDO for items.
 */
UCLASS(Blueprintable)
class PREDECESSOR_API UPredItem : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	// UPrimaryDataAsset
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    // ~UPrimaryDataAsset
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    UTexture2D* Icon;
	
    /** 
     * The Name for the item, this what will show up in the shop
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FText ItemName;

    /**
     * Short description about the item.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FText ItemDescription;

    /** 
     * Price for this item. Does not account for children.
     * If I have a child that costs 300g and I am 400g, our total is 700g. 
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    FPredItemMagnitude Price;
   
    /**
     * Items required in order to purchase this item.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    TArray<UPredItem*> RequiredItems;

    /**
     * Attributes to modify when equipping this item. 
     * If the unique identifier is already present, the effect will not be applied.
     * Will be removed when the item is removed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    TArray<FPredUniqueItemAttributeModifier> AttributeModifiers;

    /**
     * Effects to apply when this item is equipped. 
     * If the unique identifier is already present, the effect will not be applied.
     * Will be removed when the item is removed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    TArray<FPredUniqueItemEffect> ItemEffects;    

    /**
     * The active ability granted when we add this item to an entity.
     * Will not add if the ability already exists.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PredItem")
    TSubclassOf<UBaseGameplayAbility> ActiveAbility;

    UFUNCTION(BlueprintCallable, Category = "PredItem")
    FString GetIdentifierString() const;

    UFUNCTION(BlueprintPure, Category = "PredItem")
    FString GetItemName() const;

    UFUNCTION(BlueprintPure, Category = "PredItem")
    float GetTotalItemCost() const;

    UFUNCTION(BlueprintPure, Category = "PredItem")
    float GetItemCost() const;

    /**
     * Not exposed, generated at runtime to avoid designer overhead.
     * Public because we need to generate this as the items are created.
     * Doing this when we query what an item builds in to is not enough, as an item has no knowledge of what it builds in to,
     * only what builds in to it.
     */
    UPROPERTY(BlueprintReadOnly, Category = "PredItem")
    TArray<UPredItem*> BuildsInto;

    /**
     * Returns true if the inventory component (and owning actor) can buy this item.
     */
    bool CanPurchase(UPredInventoryComponent* InventoryComponent);

    /**
     * Returns the item cost for the inventory component. Takes in account parts of the item that are already owned.
     */
    float GetItemCostFor(UPredInventoryComponent* InventoryComponent);

protected:

    /*
    * Returns true if the inventory component (and owning actor) can afford this item
    */
    bool CanAfford(UPredInventoryComponent* InventoryComponent);

    /**
     * Iterates through an inventory recursively to determine if we can buy 
     */
    float GetItemCostForHelper(TArray<const UPredItem*>& RemainingInventory);

    // Cache variables to avoid repeat recursive calculations.
    mutable float CachedTotalItemCost = 0.0f;
    mutable bool HasCachedTotalItemCost = false;
};
