// Fill out your copyright notice in the Description page of Project Settings.


#include "PredInventoryComponent.h"

#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "PredAbilityLibrary.h"
#include "BaseAttributeSet.h"
#include "PredItemLibrary.h"
#include "PredCharacter.h"
#include "PredLoggingLibrary.h"
#include "PredGameplayTagLibrary.h"
#include "PredBlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "PredAbilitySystemGlobals.h"


// Sets default values for this component's properties
UPredInventoryComponent::UPredInventoryComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    // ...
}

// Called when the game starts
void UPredInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // @TODO move this elsewhere, initial gold
    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    UGameplayEffect* ItemStaticModifierGE = NewObject<UGameplayEffect>();
    ItemStaticModifierGE->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo GameplayModInfo;
    GameplayModInfo.Attribute = UBaseAttributeSet::GetGoldAttribute();
    GameplayModInfo.ModifierMagnitude = FScalableFloat(StartingGold);
    GameplayModInfo.ModifierOp = EGameplayModOp::Additive;
    ItemStaticModifierGE->Modifiers.Add(GameplayModInfo);
    OwnerASC->ApplyGameplayEffectToSelf(ItemStaticModifierGE, 1.0f, OwnerASC->MakeEffectContext());

    APredCharacter* OwnerAsPredCharacter = Cast<APredCharacter>(GetOwner());
    if (OwnerAsPredCharacter)
    {
        OwnerAsPredCharacter->OnInputComponentReady.AddDynamic(this, &UPredInventoryComponent::SetupInventoryInput);
    }


    SetupInventorySlots();
}

void UPredInventoryComponent::SetupInventorySlots()
{
    if (GetOwner()->HasAuthority())
    {
        for (int i = 0; i < NumInventorySlots; i++)
        {
            FPredInventorySlot NewSlot;
            NewSlot.SlotID = i;
            Inventory.Add(NewSlot);
        }
    }
}

void UPredInventoryComponent::SetupInventoryInput(UInputComponent* InputComponent)
{
    InputComponent->BindAction<FUseInventorySlot>("UseInventorySlotOne", IE_Pressed, this, &UPredInventoryComponent::TryUseInventorySlot, 0);
    InputComponent->BindAction<FUseInventorySlot>("UseInventorySlotTwo", IE_Pressed, this, &UPredInventoryComponent::TryUseInventorySlot, 1);
    InputComponent->BindAction<FUseInventorySlot>("UseInventorySlotThree", IE_Pressed, this, &UPredInventoryComponent::TryUseInventorySlot, 2);
    InputComponent->BindAction<FUseInventorySlot>("UseInventorySlotFour", IE_Pressed, this, &UPredInventoryComponent::TryUseInventorySlot, 3);
    InputComponent->BindAction<FUseInventorySlot>("UseInventorySlotFive", IE_Pressed, this, &UPredInventoryComponent::TryUseInventorySlot, 4);
    InputComponent->BindAction<FUseInventorySlot>("UseInventorySlotSix", IE_Pressed, this, &UPredInventoryComponent::TryUseInventorySlot, 5);
}

void UPredInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UPredInventoryComponent, Inventory);

}

int32 UPredInventoryComponent::FindSlotFromItem(const UPredItem* Item, FPredInventorySlot& OutItemSlot)
{
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        if (Inventory[i].SlottedItem.Item == Item)
        {
            OutItemSlot = Inventory[i];
            return i;
        }
    }
    return -1;
}

bool UPredInventoryComponent::HasRoomForItem(const UPredItem* Item)
{
    if (!Item)
    {
        return false;
    }

    // If we have a child item, we can replace it when we are purchased.
    // Also checking recursively against our children's children. All we need is one descendant to exist, as we can take
    // that spot after buying (we will be removing at least one instance of any given child).
    for (const UPredItem* RequiredItem : Item->RequiredItems)
    {
        if (HasItem(RequiredItem) || HasRoomForItem(RequiredItem))
        {
            return true;
        }
    }

    int32 ThrowAwayEmptySlotIdx = -1;
    return FindEmptySlot(ThrowAwayEmptySlotIdx);
}

bool UPredInventoryComponent::FindEmptySlot(int32& OutEmptySlot)
{
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        if (Inventory[i].IsEmpty())
        {
            OutEmptySlot = i;
            return true;
        }
    }
    return false;
}

bool UPredInventoryComponent::TryBuyItem(UPredItem* Item)
{
    if (CanPurchaseItem(Item,true))
    {
        UGameplayStatics::PlaySound2D(GetWorld(), UPredBlueprintFunctionLibrary::GetGlobalSoundCueByIdentifier(UPredItemLibrary::ItemShopBuyItemRowName), 1.0f, 1.f, 0.f, nullptr, GetOwner());
        Server_TryBuyItem(Item);
        return true;
    }

    return false;
}

bool UPredInventoryComponent::TrySellItem(int32 SlotToSellAt)
{
    if (UPredItemLibrary::CanSellAtInventorySlot(GetOwner(), SlotToSellAt))
    {
        UGameplayStatics::PlaySound2D(GetWorld(), UPredBlueprintFunctionLibrary::GetGlobalSoundCueByIdentifier(UPredItemLibrary::ItemShopSellItemRowName), 1.0f, 1.f, 0.f, nullptr, GetOwner());
        Server_TrySellItem(SlotToSellAt);
        return true;
    }
    return false;
}

void UPredInventoryComponent::EquipItem(UPredItem* Item, int32 Count)
{
    if (!GetOwner()->HasAuthority()) { return; }

    int32 SlotToPlaceAt = -1;
    if (FindEmptySlot(SlotToPlaceAt))
    {
        EquipItemAtSlot(Item, Count, SlotToPlaceAt);
    }
}

void UPredInventoryComponent::RemoveItem(const UPredItem* Item, int32 Count)
{
    if (!GetOwner()->HasAuthority()) { return; }

    FPredInventorySlot Slot;
    int32 ItemSlot = FindSlotFromItem(Item, Slot);
    if (ItemSlot != -1)
    {
        RemoveItemAtSlot(1, ItemSlot);
    }
}

void UPredInventoryComponent::EquipItemAtSlot(UPredItem* Item, int32 Count, int32 Slot)
{
    if (!GetOwner()->HasAuthority()) { return; }

    if (!Item) { TRACE(PredItemLog, Error, "Item was NULL when attempting to equip at slot %d", Slot); return; }

    FPredActiveItem NewItem;
    NewItem.Item = Item;

    ApplyItemEffectsToOwner(NewItem);
   
    Inventory[Slot].SlottedItem = NewItem;
    OnItemSlotUpdated.Broadcast(Inventory[Slot]);

    TRACE(PredItemLog, Log, "Item %s added to %s", *GetNameSafe(Item), *GetNameSafe(GetOwner()));
}

void UPredInventoryComponent::RemoveItemAtSlot(int32 Count, int32 Slot)
{
    if (!GetOwner()->HasAuthority()) { return; }

    FPredActiveItem ActiveItem = Inventory[Slot].SlottedItem;
    const UPredItem* Item = ActiveItem.Item;

    RemoveItemEffectsFromOwner(ActiveItem);

    FPredActiveItem EmptyItem;
    Inventory[Slot].SlottedItem = EmptyItem;
    OnItemSlotUpdated.Broadcast(Inventory[Slot]);

    RegenerateInventoryEffectsPostItemRemoval();

    TRACE(PredItemLog, Log, "Item %s removed from %s", *GetNameSafe(Item), *GetNameSafe(GetOwner()));
}

void UPredInventoryComponent::ApplyItemEffectsToOwner(FPredActiveItem& ItemToApply)
{
    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (!OwnerASC) { return; }

    const UPredItem* Item = ItemToApply.Item;
    
    FGameplayEffectSpecHandle MultiplicativeEffectSpec = UPredAbilityLibrary::MakeOutgoingMultiplicativeEffectSpec(OwnerASC->MakeEffectContext());
    bool bHasMultiplicative = false;

    // Apply item's static mods.
    for (const FPredUniqueItemAttributeModifier& UniqueAttributeModifier : Item->AttributeModifiers)
    {
        // We already have this unique effect applied, skip.
        if (UniqueAttributeModifier.UniqueIdentifier != FGameplayTag::EmptyTag && IsUniqueIdentifierApplied(UniqueAttributeModifier.UniqueIdentifier))
        {
            continue;
        }

        if (UniqueAttributeModifier.UniqueIdentifier != FGameplayTag::EmptyTag)
        {
            UniqueProviders.Add(UniqueAttributeModifier.UniqueIdentifier, ItemToApply);
        }

        FPredItemAttributeModifier ItemAttributeModifier = UniqueAttributeModifier.AttributeModifier;
        if (ItemAttributeModifier.AttributeModType == EPredItemAttributeModType::Multiply)
        {
            bHasMultiplicative = true;
            UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(MultiplicativeEffectSpec, UPredAbilityLibrary::GetSetByCallerTagForAttribute(ItemAttributeModifier.Attribute), ItemAttributeModifier.GetMagnitude());
        }
        else
        {
            OwnerASC->ApplyModToAttribute(ItemAttributeModifier.Attribute, EGameplayModOp::Additive, ItemAttributeModifier.GetMagnitude());
        }
    }
    if (bHasMultiplicative)
    {
        FActiveGameplayEffectHandle MultiplicativeStatsHandle = OwnerASC->BP_ApplyGameplayEffectSpecToSelf(MultiplicativeEffectSpec);
        ItemToApply.ActiveEffects.Add(MultiplicativeStatsHandle);
    }
    
    // Item effects
    for (const FPredUniqueItemEffect& UniqueItemEffect : Item->ItemEffects)
    {
        // We already have this effect identifier applied.
        if (UniqueItemEffect.UniqueIdentifier != FGameplayTag::EmptyTag && IsUniqueIdentifierApplied(UniqueItemEffect.UniqueIdentifier))
        {
            continue;
        }

        if (UniqueItemEffect.UniqueIdentifier != FGameplayTag::EmptyTag)
        {
            UniqueProviders.Add(UniqueItemEffect.UniqueIdentifier, ItemToApply);
        }

        FActiveGameplayEffectHandle ItemEffectHandle = OwnerASC->BP_ApplyGameplayEffectToSelf(UniqueItemEffect.UniqueEffect, 1.0f, OwnerASC->MakeEffectContext());
        ItemToApply.ActiveEffects.Add(ItemEffectHandle);
    }
}

void UPredInventoryComponent::RemoveItemEffectsFromOwner(FPredActiveItem& ActiveItemToRemove)
{
    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (!OwnerASC) { return; }

    const UPredItem* Item = ActiveItemToRemove.Item;
    for (const FPredUniqueItemAttributeModifier& UniqueAttributeModifier : Item->AttributeModifiers)
    {
        // If we are a uniquely specified attribute mod, and this item isn't applying that mod, continue.
        if (UniqueAttributeModifier.UniqueIdentifier != FGameplayTag::EmptyTag && !IsProviderOfUniqueEffect(ActiveItemToRemove,UniqueAttributeModifier.UniqueIdentifier))
        {
            continue;
        }

        if (UniqueAttributeModifier.UniqueIdentifier != FGameplayTag::EmptyTag)
        {
            UniqueProviders.Remove(UniqueAttributeModifier.UniqueIdentifier);
        }

        FPredItemAttributeModifier AttributeMod = UniqueAttributeModifier.AttributeModifier;

        if (AttributeMod.AttributeModType == EPredItemAttributeModType::Add)
        {
            OwnerASC->ApplyModToAttribute(AttributeMod.Attribute, EGameplayModOp::Additive, (-1 * AttributeMod.GetMagnitude()));
        }
    }

    // This will catch multiply changes as well.
    for (FActiveGameplayEffectHandle& ActiveItemEffect : ActiveItemToRemove.ActiveEffects)
    {
        OwnerASC->RemoveActiveGameplayEffect(ActiveItemEffect);
    }
}

void UPredInventoryComponent::RegenerateInventoryEffectsPostItemRemoval()
{
    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

    for (FPredInventorySlot& InventorySlot : Inventory)
    {
        if (InventorySlot.IsEmpty()) { continue; }

        FPredActiveItem& ActiveItem = InventorySlot.SlottedItem;
        const UPredItem* ItemDef = ActiveItem.Item;

        FGameplayEffectSpecHandle MultiplicativeEffectSpec = UPredAbilityLibrary::MakeOutgoingMultiplicativeEffectSpec(OwnerASC->MakeEffectContext());
        bool bHasMultiplicative = false;

        // Static mods
        for (const FPredUniqueItemAttributeModifier& UniqueAttributeModifier : ItemDef->AttributeModifiers)
        {
            // If we aren't a unique attribute, or we already have an instance of this unique identifier applied, continue.
            // These were applied when we equipped the item (or some other item).
            if (UniqueAttributeModifier.UniqueIdentifier == FGameplayTag::EmptyTag || IsUniqueIdentifierApplied(UniqueAttributeModifier.UniqueIdentifier))
            {
                continue;
            }

            if (UniqueAttributeModifier.UniqueIdentifier != FGameplayTag::EmptyTag)
            {
                UniqueProviders.Add(UniqueAttributeModifier.UniqueIdentifier, ActiveItem);
            }

            FPredItemAttributeModifier AttributeModifier = UniqueAttributeModifier.AttributeModifier;
            if (AttributeModifier.AttributeModType == EPredItemAttributeModType::Add)
            {
                OwnerASC->ApplyModToAttribute(AttributeModifier.Attribute, EGameplayModOp::Additive, (AttributeModifier.GetMagnitude()));
            }
            if (AttributeModifier.AttributeModType == EPredItemAttributeModType::Multiply)
            {
                bHasMultiplicative = true;
                UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(MultiplicativeEffectSpec, UPredAbilityLibrary::GetSetByCallerTagForAttribute(AttributeModifier.Attribute), AttributeModifier.GetMagnitude());
            }
        }
        if (bHasMultiplicative)
        {
            FActiveGameplayEffectHandle MultiplicativeHandle = OwnerASC->BP_ApplyGameplayEffectSpecToSelf(MultiplicativeEffectSpec);
            ActiveItem.ActiveEffects.Add(MultiplicativeHandle);
        }

        // Item Effects
        for (const FPredUniqueItemEffect& UniqueItemEffect : ItemDef->ItemEffects)
        {
            if (UniqueItemEffect.UniqueIdentifier == FGameplayTag::EmptyTag || IsUniqueIdentifierApplied(UniqueItemEffect.UniqueIdentifier))
            {
                continue;
            }

            if (UniqueItemEffect.UniqueIdentifier != FGameplayTag::EmptyTag)
            {
                UniqueProviders.Add(UniqueItemEffect.UniqueIdentifier, ActiveItem);
            }

            FGameplayEffectSpecHandle ItemEffectSpecHande = OwnerASC->MakeOutgoingSpec(UniqueItemEffect.UniqueEffect, 1.0f, OwnerASC->MakeEffectContext());
            FActiveGameplayEffectHandle ActiveItemEffectHandle = OwnerASC->BP_ApplyGameplayEffectSpecToSelf(ItemEffectSpecHande);
            ActiveItem.ActiveUniqueEffects.Add(FPredActiveUniqueEffect(UniqueItemEffect.UniqueIdentifier, ActiveItemEffectHandle));
            UniqueProviders.Add(UniqueItemEffect.UniqueIdentifier, ActiveItem);

        }
    }
}


bool UPredInventoryComponent::GetInventorySlotAt(int32 SlotIndex, FPredInventorySlot& OutSlot)
{
    if (Inventory.IsValidIndex(SlotIndex))
    {
        OutSlot = Inventory[SlotIndex];
        return true;
    }
    return false;
}

void UPredInventoryComponent::GetAllInventorySlots(TArray<FPredInventorySlot>& Slots)
{
    Slots = Inventory;
}

float UPredInventoryComponent::CalculateItemCost(UPredItem* Item)
{
    return Item->GetItemCostFor(this);
}

float UPredInventoryComponent::GetItemSellPrice(const UPredItem* Item)
{
    // Selling for a fraction of the cost.
    return Item->GetTotalItemCost() * SellModifier;
}

bool UPredInventoryComponent::IsProviderOfUniqueEffect(const FPredActiveItem& Item, FGameplayTag UniqueEffectIdentifier)
{
    return IsUniqueIdentifierApplied(UniqueEffectIdentifier) && UniqueProviders[UniqueEffectIdentifier].UniqueItemID == Item.UniqueItemID;
}

bool UPredInventoryComponent::IsUniqueIdentifierApplied(const FGameplayTag& EffectIdentifier)
{
    return UniqueProviders.Contains(EffectIdentifier);
}

bool UPredInventoryComponent::CanSellAtInventorySlot(int32 SlotID)
{
    FPredInventorySlot& InventorySlot = Inventory[SlotID];
    return !InventorySlot.IsEmpty();
}

float UPredInventoryComponent::GetItemPriceAtInventorySlot(int32 SlotID)
{
    FPredInventorySlot& InventorySlot = Inventory[SlotID];
    return InventorySlot.IsEmpty() ? 0.0f : GetItemSellPrice(Inventory[SlotID].SlottedItem.Item);
}

int32 UPredInventoryComponent::GetItemCount(const UPredItem* Item)
{
    int32 ReturnedCount = 0;
    for (FPredInventorySlot& ItemSlot : Inventory)
    {
        if (ItemSlot.SlottedItem.Item == Item)
        {
            ReturnedCount++;
        }
    }
    return ReturnedCount;
}

bool UPredInventoryComponent::HasItem(const UPredItem* Item)
{
    return GetItemCount(Item) > 0;
}

void UPredInventoryComponent::TryUseInventorySlot(int32 Idx)
{
    if (!Inventory.IsValidIndex(Idx))
    {
        TRACE(PredItemLog, Error, "Tried to use item at %d, but %d was not a valid inventory index.", Idx);
        return;
    }

    if (Inventory[Idx].IsEmpty() || !Inventory[Idx].SlottedItem.ActiveAbility.IsValid())
    {
        return;
    }

    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (OwnerASC)
    {
        OwnerASC->TryActivateAbility(Inventory[Idx].SlottedItem.ActiveAbility);
    }

}

bool UPredInventoryComponent::CanPurchaseItem(UPredItem* Item, bool bUseLocation)
{
     FGameplayTagContainer BuyTagContainer;

     BuyTagContainer.AddTag(PredGlobalTags::Dead());
     BuyTagContainer.AddTag(PredGlobalTags::LocationShop());
     bool OwnerHasTags = (UPredGameplayTagLibrary::HasAnyMatchingGameplayTags(GetOwner(), BuyTagContainer) || bUseLocation == false);

    return HasRoomForItem(Item) && Item->CanPurchase(this) && OwnerHasTags;
}

void UPredInventoryComponent::Server_TryBuyItem_Implementation(UPredItem* Item)
{
    if (CanPurchaseItem(Item))
    {
        // Determine how much we're paying for this item, before we remove the children
        // (and making it more expensive)
        float ItemCost = Item->GetItemCostFor(this);

        // Clear out the inventory of any required items we had completed, will also find partially completed required items.
        ClearInventoryPostPurchase(Item);

        // Equip the item.
        EquipItem(Item, 1.0);

        // Apply cost
        UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
        if (!OwnerASC) { TRACE(PredItemLog, Error, "Tried to charge %s for %s, but could not find AbilitySystemComponent"); return; }

        UGameplayEffect* ItemStaticModifierGE = NewObject<UGameplayEffect>();
        ItemStaticModifierGE->DurationPolicy = EGameplayEffectDurationType::Instant;

        FGameplayModifierInfo GameplayModInfo;
        GameplayModInfo.Attribute = UBaseAttributeSet::GetGoldAttribute();
        GameplayModInfo.ModifierMagnitude = FScalableFloat(-1 * ItemCost);
        GameplayModInfo.ModifierOp = EGameplayModOp::Additive;

        ItemStaticModifierGE->Modifiers.Add(GameplayModInfo);

        OwnerASC->ApplyGameplayEffectToSelf(ItemStaticModifierGE, 1.0f, OwnerASC->MakeEffectContext());

        TRACE(PredItemLog, Log, "%s purchased item %s for %f gold.", *GetNameSafe(GetOwner()), *Item->ItemName.ToString(), ItemCost);
    }
}

void UPredInventoryComponent::ClearInventoryPostPurchase(const UPredItem* Item)
{
    // We don't remove the passed in item, as that is what we are buying.
    // What we do want to remove is the item's children (if we own the child) or any part of a child item.
    for (const UPredItem* ChildItem : Item->RequiredItems)
    {
        ClearInventoryPostPurchaseHelper(ChildItem);
    }
}

void UPredInventoryComponent::ClearInventoryPostPurchaseHelper(const UPredItem* ChildItem)
{
    // If we have this child item, we remove it and we're done.
    // Don't want to remove any children of this node.
    if (HasItem(ChildItem))
    {
        RemoveItem(ChildItem, 1);
        return;
    }

    // Otherwise, need to check if we have any of the children of this item, recursively.
    for (const UPredItem* GrandChildItem : ChildItem->RequiredItems)
    {
        ClearInventoryPostPurchaseHelper(GrandChildItem);
    }
}

bool UPredInventoryComponent::Server_TryBuyItem_Validate(UPredItem* Item)
{
    return true;
}

void UPredInventoryComponent::Server_TrySellItem_Implementation(int32 ItemSlot)
{
    if (UPredItemLibrary::CanSellAtInventorySlot(GetOwner(), ItemSlot))
    {

        UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
        if (!OwnerASC) { /** yikes */ return; }

        const UPredItem* ItemAtSlot = Inventory[ItemSlot].SlottedItem.Item;
        int32 ItemSellPrice = FMath::FloorToInt(GetItemSellPrice(ItemAtSlot));

        UGameplayEffect* ItemStaticModifierGE = NewObject<UGameplayEffect>();
        ItemStaticModifierGE->DurationPolicy = EGameplayEffectDurationType::Instant;

        FGameplayModifierInfo GameplayModInfo;
        GameplayModInfo.Attribute = UBaseAttributeSet::GetGoldAttribute();
        GameplayModInfo.ModifierMagnitude = FScalableFloat(ItemSellPrice);
        GameplayModInfo.ModifierOp = EGameplayModOp::Additive;

        ItemStaticModifierGE->Modifiers.Add(GameplayModInfo);

        OwnerASC->ApplyGameplayEffectToSelf(ItemStaticModifierGE, 1.0f, OwnerASC->MakeEffectContext());

        RemoveItemAtSlot(1, ItemSlot);

        TRACE(PredItemLog, Log, "%s sold item %s for %d gold.", *GetNameSafe(GetOwner()), *ItemAtSlot->ItemName.ToString(), ItemSellPrice);
    }
}

bool UPredInventoryComponent::Server_TrySellItem_Validate(int32 ItemSlot)
{
    return true;
}

void UPredInventoryComponent::OnRep_Inventory()
{
    for (FPredInventorySlot& Slot : Inventory)
    {
        OnItemSlotUpdated.Broadcast(Slot);
    }
}

//////////////////////////////////////////////////////////////////////////
// Debug
//////////////////////////////////////////////////////////////////////////

void UPredInventoryComponent::GenerateDebugString(FString& OutDebugString)
{
    OutDebugString = "";

    OutDebugString += FString::Printf(TEXT("Inventory for %s: \n"), *GetNameSafe(GetOwner()));
    for (int i = 0; i < Inventory.Num(); i++)
    {
        OutDebugString += FString::Printf(TEXT("%s,"), Inventory[i].IsEmpty() ? TEXT("Empty") : *Inventory[i].SlottedItem.Item->GetItemName());
    }
}
